#include <Arduino.h>
#include "config.h"
#include "config_arduino.h"
#include "types.h"
#include "tasks.h"
#include "scheduler.h"
#include "datastore.h"
#include "debug_print.h"
#include "debug_console.h"
#include "bench_setup.h"
#include "fotores_drivers.h"
#include "uv_drivers.h"
#include "gybmep_drivers.h"
#include "fuel_gauge_drivers.h"
#include "mq_7_drivers.h"
#include "sd_drivers.h"
#include "lora_drivers.h"

/*
Each core owns its scheduler and is the only one touching it.
The mission state task (core 0) only writes the flight state in the datastore,
then every core applies its own tasks profile as soon as it sees the state changed.
This way on_enable/on_disable always run on the core that owns the task.
*/

static scheduler_t sched_core0;
static scheduler_t sched_core1;

#ifdef BENCH_TEST
#define BENCH_SERIAL_WAIT_MS 5000

// USB serial drops what's printed before the monitor opens: wait a bit so the init messages are seen
static void wait_serial_monitor(void)
{
  while (!Serial && millis() < BENCH_SERIAL_WAIT_MS)
    ;
}
#else

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

/*
Inits only prepare the hardware (most sensors are left asleep): tasks are enabled by the flight state profiles.
A failed init doesn't stop the boot, the task will count the failures once its profile enables it.
*/

// Core 0 sensors
static void sensors_init(void)
{
  // First: the SOC estimate needs the battery at rest, before any big load is on (blocks ~750 ms)
  if (fuel_gauge_init() == SENS_STATE_ERR)
    DBG_ERR("FUEL_GAUGE init failed");

  fotores_init();
  uv_init();

  // Left asleep, the ENV task wakes it up
  if (gybmep_init() != SENS_STATE_READY)
    DBG_ERR("GYBMEP init failed");

  // Heater off, the GAS task turns it on
  mq_7_init();

  // No init for:
  // - GPS: gps_wake (on_enable) powers it on. Powering it on at boot browns out the board
  // - IMU: imu_wake (on_enable) starts it
  // - MLX90640: mlx_init is its on_enable
}

// Core 1 (SD / LoRa), both on the shared SPI bus owned by core 1
static void sensors_init1(void)
{
  // FAT 8.3 names only (max 8 chars + 3 of extension)
  if (sd_init("/DATA", "/DATA/LOG.BIN") != SENS_STATE_READY)
    DBG_ERR("SD init failed");

  // Sender, left asleep: the LORA task wakes it up
  if (lora_init(false) != SENS_STATE_READY)
    DBG_ERR("LORA init failed");
}
#endif

void setup()
{
  Serial.begin(115200);

  sched_init(&sched_core0, tasks_core0, TASK_CORE0_COUNT, tasks_enabled_core0);
  debug_console_init(&sched_core0, &sched_core1);

#ifdef BENCH_TEST
  wait_serial_monitor();
  bench_setup(&sched_core0);
#else
  sensors_init();
#endif
}

void loop()
{
#ifndef BENCH_TEST
  static flight_state_t applied = FLIGHT_STATE_COUNT; // forces the first apply
  apply_profile_if_changed(&sched_core0, tasks_profile_core0, &applied);
#endif
  sched_tick(&sched_core0);
  debug_console_poll_core0();
}

void setup1()
{
  sched_init(&sched_core1, tasks_core1, TASK_CORE1_COUNT, tasks_enabled_core1);

#ifdef BENCH_TEST
  wait_serial_monitor();
  bench_setup1(&sched_core1);
#else
  sensors_init1();
#endif
}

void loop1()
{
#ifndef BENCH_TEST
  static flight_state_t applied = FLIGHT_STATE_COUNT; // forces the first apply
  apply_profile_if_changed(&sched_core1, tasks_profile_core1, &applied);
#endif
  debug_console_poll_core1();
  sched_tick(&sched_core1);
}
