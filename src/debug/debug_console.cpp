#include "debug_console.h"

#ifdef SERIAL_DEBUG_COMMANDS

#include <Arduino.h>
#include "config.h"
#include "tasks.h"
#include "scheduler.h"
#include "datastore.h"
#include "gps_drivers.h"

#define DATASTORE_PRINT_INTERVAL_MS 2000

// A write from another task can make ds_read_all fail, so the d command retries a few times
#define DATASTORE_PRINT_ONCE_TRIES 10

// Same threshold the UV driver uses to set is_there_light
#define FAKE_UV_LIGHT_THRESH 2.0f

static scheduler_t *sched_core0;
static scheduler_t *sched_core1;

/*
Fake sensor values set by the l / u commands. They are written again in the datastore after
every sched_tick of core 0, so the real sensor tasks can't overwrite them before the
mission state task reads them (they all run on core 0).
*/
static bool fake_light_on = false;
static bool fake_light_value;
static bool fake_uv_on = false;
static float fake_uv_index;

// Task of core 1 to toggle: set by core 0, consumed by core 1 since it owns its scheduler
static volatile int16_t core1_toggle_request = -1;

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
  d1/d0  turns the periodic datastore print on/off (off at boot)
  g1/g0  turns the echo of the raw GPS NMEA lines on/off (off at boot, prints only while the GPS is on)
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
    print_scheduler(sched_core0);
    print_scheduler(sched_core1);
  }
  else if (cmd[0] == 't' && (index = parse_index(cmd + 1, TASK_CORE0_COUNT)) >= 0)
  {
    toggle_task(sched_core0, (size_t)index);
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
  else if (cmd[0] == 'g' && (cmd[1] == '0' || cmd[1] == '1') && cmd[2] == '\0')
  {
    gps_set_raw_echo(cmd[1] == '1');
    DBG_INFO("CMD GPS raw NMEA echo %s", cmd[1] == '1' ? "on" : "off");
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
    DBG_ERR("unknown command '%s' (s<0-%d>, p, t<0-%d>, T<0-%d>, l1/l0/l, u<X>/u, d/d1/d0, g1/g0)",
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

void debug_console_init(scheduler_t *core0, scheduler_t *core1)
{
  sched_core0 = core0;
  sched_core1 = core1;
}

void debug_console_poll_core0(void)
{
  apply_fake_values();
  handle_serial_commands();
  print_datastore_periodically();
}

void debug_console_poll_core1(void)
{
  int16_t req = core1_toggle_request;
  if (req >= 0)
  {
    core1_toggle_request = -1;
    toggle_task(sched_core1, (size_t)req);
  }
}

#endif
