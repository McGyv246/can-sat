#include <Arduino.h>
#include "config.h"
#include "config_arduino.h"
#include "types.h"
#include "tasks.h"
#include "scheduler.h"
#include "datastore.h"

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
  }
}

void setup()
{
  Serial.begin(115200);

  // TODO: init sensors / drivers here

  sched_init(&sched_core0, tasks_core0, TASK_CORE0_COUNT, tasks_enabled_core0);
}

void loop()
{
  static flight_state_t applied = FLIGHT_STATE_COUNT; // forces the first apply
  apply_profile_if_changed(&sched_core0, tasks_profile_core0, &applied);
  sched_tick(&sched_core0);
}

void setup1()
{
  // TODO: init sd / lora here

  sched_init(&sched_core1, tasks_core1, TASK_CORE1_COUNT, tasks_enabled_core1);
}

void loop1()
{
  static flight_state_t applied = FLIGHT_STATE_COUNT; // forces the first apply
  apply_profile_if_changed(&sched_core1, tasks_profile_core1, &applied);
  sched_tick(&sched_core1);
}
