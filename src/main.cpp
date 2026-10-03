#include <Arduino.h>
#include "config.h"
#include "config_arduino.h"
#include "types.h"
#include "tasks.h"
#include "scheduler.h"
#include "datastore.h"
#include "debug_print.h"
#include "fotores_drivers.h"
#include "uv_drivers.h"
#include "gybmep_drivers.h"
#include "gps_drivers.h"
#include "fuel_gauge_drivers.h"
#include "mq_7_drivers.h"
#include "lora_drivers.h"
#include "sd_drivers.h"

#define DATASTORE_PRINT_INTERVAL_MS 2000

/*
Each core owns its scheduler and is the only one touching it.
The mission state task (core 0) only writes the flight state in the datastore,
then every core applies its own tasks profile as soon as it sees the state changed.
This way on_enable/on_disable always run on the core that owns the task.
*/

static scheduler_t sched_core0;
static scheduler_t sched_core1;

// Applies the tasks profile of the current flight state if it changed since the last call
static void apply_profile_if_changed(scheduler_t *sched, const bool *const *profiles, flight_state_t *applied)
{
  flight_state_t state = ds_read_state();
  if (state != *applied)
  {
    sched_change_enabled_tasks(sched, profiles[state]);
    *applied = state;
    // Compare its time with the "STATE" line of core 0 to see how long each core takes to follow
    DBG_INFO("tasks profile of %s applied", debug_flight_state_name(state));
  }
}

#ifdef SERIAL_DEBUG_DATA

// Turned on/off by the d1 / d0 commands
static bool datastore_print_enabled = false;

// Dumps the whole datastore on Serial, returns false if a write was in progress and nothing was printed
static bool print_datastore_now(void)
{
  datastore_t snapshot;
  if (!ds_read_all(&snapshot))
    return false;

  DBG_DATA(print_datastore, &snapshot);
  return true;
}

#endif

#ifdef SERIAL_DEBUG_COMMANDS

// A write from another task can make ds_read_all fail, so the d command retries a few times
#define DATASTORE_PRINT_ONCE_TRIES 10

// Same threshold the UV driver uses to set is_there_light
#define FAKE_UV_LIGHT_THRESH 2.0f

/*
Fake sensor values set by the l / u commands. They are written again in the datastore after
every sched_tick of core 0, so the real sensor tasks can't overwrite them before the
mission state task reads them (they all run on core 0).
*/
static bool fake_light_on = false;
static bool fake_light_value;
static bool fake_uv_on = false;
static float fake_uv_index;

// Task of core 1 to toggle: set by core 0, consumed by loop1 since core 1 owns its scheduler
static volatile int16_t core1_toggle_request = -1;

static void toggle_task(scheduler_t *sched, size_t index)
{
  bool enabled = !sched->enabled_tasks[index];
  sched_change_enabled_i(sched, enabled, index);
  DBG_INFO("CMD task %s %s", sched->tasks[index].name, enabled ? "enabled" : "disabled");
}

static void apply_fake_values(void)
{
  uint32_t now = millis();

  if (fake_light_on)
  {
    fotores_t f = {};
    f.s.t_ms = now;
    f.s.valid = true;
    f.is_there_light = fake_light_value;
    ds_write_fotores(&f, now);
  }

  if (fake_uv_on)
  {
    uv_t u = {};
    u.s.t_ms = now;
    u.s.valid = true;
    u.is_there_light = fake_uv_index > FAKE_UV_LIGHT_THRESH;
    u.uv_index = fake_uv_index;
    ds_write_uv(&u, now);
  }
}

// Returns the number in s if it's a valid index below count, -1 otherwise
static int parse_index(const char *s, size_t count)
{
  char *end;
  unsigned long n = strtoul(s, &end, 10);
  if (end == s || *end != '\0' || n >= count)
    return -1;
  return (int)n;
}

/*
Commands for bench tests, one per line (read on core 0):
  s<N>   forces the flight state N, e.g. s4 = DESCENT (the mission state task keeps running from there)
  p      prints both schedulers
  t<N>   turns task N of core 0 on/off (index in task_core0_id_t), e.g. t3 = GAS
  T<N>   turns task N of core 1 on/off (index in task_core1_id_t), e.g. T1 = LORA
  l1/l0  fakes light / darkness on the photoresistor, l alone stops faking
  u<X>   fakes UV index X, e.g. u3.5 (light if > 2), u alone stops faking
  d      prints the datastore once (needs SERIAL_DEBUG_DATA, like d1/d0)
  d1/d0  turns the periodic datastore print on/off (on at boot)
The tasks profile applied on every state change overrides the t/T toggles.
*/
static void execute_command(const char *cmd)
{
  int index;

  if (cmd[0] == 's' && cmd[1] >= '0' && cmd[1] < '0' + FLIGHT_STATE_COUNT && cmd[2] == '\0')
  {
    flight_state_t state = (flight_state_t)(cmd[1] - '0');
    DBG_INFO("CMD forcing state %s", debug_flight_state_name(state));
    ds_write_flight_state(state, millis());
  }
  else if (cmd[0] == 'p' && cmd[1] == '\0')
  {
    // Core 1 may be changing its scheduler meanwhile: its values can be slightly inconsistent
    print_scheduler(&sched_core0);
    print_scheduler(&sched_core1);
  }
  else if (cmd[0] == 't' && (index = parse_index(cmd + 1, TASK_CORE0_COUNT)) >= 0)
  {
    toggle_task(&sched_core0, (size_t)index);
  }
  else if (cmd[0] == 'T' && (index = parse_index(cmd + 1, TASK_CORE1_COUNT)) >= 0)
  {
    core1_toggle_request = (int16_t)index;
  }
  else if (cmd[0] == 'l' && (cmd[1] == '\0' || ((cmd[1] == '0' || cmd[1] == '1') && cmd[2] == '\0')))
  {
    fake_light_on = cmd[1] != '\0';
    fake_light_value = cmd[1] == '1';
    if (fake_light_on)
      DBG_INFO("CMD faking %s on the photoresistor", fake_light_value ? "light" : "darkness");
    else
      DBG_INFO("CMD photoresistor fake stopped");
  }
  else if (cmd[0] == 'u' && cmd[1] == '\0')
  {
    fake_uv_on = false;
    DBG_INFO("CMD UV fake stopped");
  }
  else if (cmd[0] == 'u')
  {
    char *end;
    float value = strtof(cmd + 1, &end);
    if (end == cmd + 1 || *end != '\0' || value < 0)
    {
      DBG_ERR("bad UV index '%s'", cmd + 1);
      return;
    }
    fake_uv_on = true;
    fake_uv_index = value;
    // the log printf can't format floats (%f prints nothing): integers only
    unsigned value_x100 = (unsigned)(value * 100.0f + 0.5f);
    DBG_INFO("CMD faking UV index %u.%02u (%s)", value_x100 / 100, value_x100 % 100, value > FAKE_UV_LIGHT_THRESH ? "light" : "dark");
  }
#ifdef SERIAL_DEBUG_DATA
  else if (cmd[0] == 'd' && cmd[1] == '\0')
  {
    int tries = 0;
    while (!print_datastore_now() && ++tries < DATASTORE_PRINT_ONCE_TRIES)
      ;
    if (tries == DATASTORE_PRINT_ONCE_TRIES)
      DBG_ERR("datastore busy, retry");
  }
  else if (cmd[0] == 'd' && (cmd[1] == '0' || cmd[1] == '1') && cmd[2] == '\0')
  {
    datastore_print_enabled = cmd[1] == '1';
    DBG_INFO("CMD periodic datastore print %s", datastore_print_enabled ? "on" : "off");
  }
#endif
  else
  {
    DBG_ERR("unknown command '%s' (s<0-%d>, p, t<0-%d>, T<0-%d>, l1/l0/l, u<X>/u, d/d1/d0)",
            cmd, FLIGHT_STATE_COUNT - 1, TASK_CORE0_COUNT - 1, TASK_CORE1_COUNT - 1);
  }
}

// Collects the characters from Serial (they can arrive one at a time) and runs each line as a command
static void handle_serial_commands(void)
{
  static char cmd[16];
  static size_t len = 0;

  while (Serial.available())
  {
    char c = (char)Serial.read();
    if (c != '\n' && c != '\r')
    {
      if (len < sizeof(cmd) - 1)
        cmd[len++] = c;
      continue;
    }
    if (len == 0)
      continue; // empty line, or the '\n' after a '\r'

    cmd[len] = '\0';
    len = 0;
    execute_command(cmd);
  }
}

#endif

// Dumps the whole datastore on Serial every DATASTORE_PRINT_INTERVAL_MS (core 0 only, to avoid mixed output)
static void print_datastore_periodically(void)
{
#ifdef SERIAL_DEBUG_DATA
  static uint32_t last_print = 0;
  uint32_t now = millis();
  if (!datastore_print_enabled || now - last_print < DATASTORE_PRINT_INTERVAL_MS)
    return;

  if (!print_datastore_now())
    return; // a write was in progress, retry on the next loop

  last_print = now;
#endif
}

#ifdef BENCH_TEST
#define BENCH_SERIAL_WAIT_MS 5000

/*
Bench test of single sensors: init only the sensors under test and enable only their tasks.
Flight state profiles are never applied, so nothing else gets enabled (the t/T commands still work).
To add a sensor call its init and enable its task here, comment out the ones you don't want to run.
*/
static void bench_setup(void)
{
  fotores_init();
  sched_change_enabled_i(&sched_core0, true, TASK_CORE0_FOTORES_SLOW);

  // GUVA-S12SD on A0 (D14), powered from 3.3 V so its output stays inside the ADC range
  uv_init();
  sched_change_enabled_i(&sched_core0, true, TASK_CORE0_UV_SLOW);

  // BME280 on I2C (A4 SDA / A5 SCL). init leaves it asleep, enabling the task wakes it up
  if (gybmep_init() == SENS_STATE_READY)
    sched_change_enabled_i(&sched_core0, true, TASK_CORE0_ENV);
  else
    DBG_ERR("GYBMEP init failed, task not enabled");

  // LSM6DSOX on the Nano board itself (I2C 0x6A), no init: enabling the task starts it
  sched_change_enabled_i(&sched_core0, true, TASK_CORE0_IMU);

  // NEO-6M on Serial1 (GPS TX -> D0/RX, GPS RX -> D1/TX), 9600 baud. init powers it on and sets the
  // command pin, it returns NOT_READY until there's a fix, that's expected here
  // Disabled: turning the GPS on at boot browns out the board. Use t0 to turn it on at runtime
  // gps_init();
  // sched_change_enabled_i(&sched_core0, true, TASK_CORE0_GPS);

  // MAX1704X on I2C (0x36), powered by the battery cell: it only answers with the battery connected.
  // init leaves it asleep, enabling the task wakes it up
  if (fuel_gauge_init() != SENS_STATE_ERR)
    sched_change_enabled_i(&sched_core0, true, TASK_CORE0_BATTERY);
  else
    DBG_ERR("FUEL_GAUGE init failed, task not enabled");

  // MQ-7 on A1 (D15) through the voltage divider, heater switched by D3. init leaves the heater off.
  // The task (which turns the heater on, ~150 mA at 5 V) is not enabled at boot because of the brownouts
  // seen with the GPS: use t3 to turn it on/off at runtime
  mq_7_init();
}

/*
Bench test of core 1 (SD / LoRa): runs in setup1 because core 1 owns its scheduler.
*/
static void bench_setup1(void)
{
  // SD module on SPI (CS D7), shared with LoRa. The file is opened in append mode, so data from previous
  // runs is kept. The task writes what the sensor tasks put in the ring buffer (pack_data_sd format).
  // FAT 8.3 names only (max 8 chars + 3 of extension)
  if (sd_init("/DATA", "/DATA/LOG.BIN") == SENS_STATE_READY)
    sched_change_enabled_i(&sched_core1, true, TASK_CORE1_SD);
  else
    DBG_ERR("SD init failed, task not enabled");

  // SX1276 on SPI (CS D9, DIO0 D8, RESET D10) as sender. init leaves it asleep, enabling the task wakes it up.
  // The task sends the whole datastore packed by ds_to_lora every period
  if (lora_init(false) == SENS_STATE_READY)
    sched_change_enabled_i(&sched_core1, false, TASK_CORE1_LORA);
  else
    DBG_ERR("LORA init failed, task not enabled");
}
#endif

void setup()
{
  Serial.begin(115200);

  // TODO: init sensors / drivers here

  sched_init(&sched_core0, tasks_core0, TASK_CORE0_COUNT, tasks_enabled_core0);

#ifdef BENCH_TEST
  // USB serial drops what's printed before the monitor opens: wait a bit so the init messages are seen
  while (!Serial && millis() < BENCH_SERIAL_WAIT_MS)
    ;
  bench_setup();
#endif
}

void loop()
{
#ifndef BENCH_TEST
  static flight_state_t applied = FLIGHT_STATE_COUNT; // forces the first apply
  apply_profile_if_changed(&sched_core0, tasks_profile_core0, &applied);
#endif
  sched_tick(&sched_core0);
#ifdef SERIAL_DEBUG_COMMANDS
  apply_fake_values();
  handle_serial_commands();
#endif
  print_datastore_periodically();
}

void setup1()
{
  // TODO: init sd / lora here

  sched_init(&sched_core1, tasks_core1, TASK_CORE1_COUNT, tasks_enabled_core1);

#ifdef BENCH_TEST
  // Same wait as setup(), so the init messages of core 1 aren't lost either
  while (!Serial && millis() < BENCH_SERIAL_WAIT_MS)
    ;
  bench_setup1();
#endif
}

void loop1()
{
#ifndef BENCH_TEST
  static flight_state_t applied = FLIGHT_STATE_COUNT; // forces the first apply
  apply_profile_if_changed(&sched_core1, tasks_profile_core1, &applied);
#endif
#ifdef SERIAL_DEBUG_COMMANDS
  int16_t req = core1_toggle_request;
  if (req >= 0)
  {
    core1_toggle_request = -1;
    toggle_task(&sched_core1, (size_t)req);
  }
#endif
  sched_tick(&sched_core1);
}
