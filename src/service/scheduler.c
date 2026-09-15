#include <stdint.h>
#include "types.h"

// To be moved in types.h
typedef struct
{
    task_t *tasks;
    uint16_t tasks_number;
    uint16_t max_exec_time;
    uint64_t run_count;
    uint8_t *enabled_tasks; // Eventually change type (to something like bool ig), array that defines if a task is enabled or not.
} scheduler_t;

/*
Tasks (so that I can define the friggin structure):
- sensor reads
- phase change check
- (watchdog?)
-
*/
typedef struct
{
    // This structure is correct for sensors, but it could create problems if I wanted to put some different tasks to run
    // Maybe I could just wrap sensors in some sort of runnable thing, but I don't know how I could get the sleep and init functions,
    // while poll and get can be easily wrapped in a function that checks the return status and then gets automatically the value.
    uint32_t (*run)(void);
    void (*on_enable)(void);  /* NULL if nothing to do */
    void (*on_disable)(void); /* NULL if nothing to do */
    sens_status_t last_status;
    const char *name;
    uint16_t period;
    uint64_t due;
    uint16_t priority;

} task_t;

void sched_init(scheduler_t *sched, task_t *tasks, uint16_t tasks_number)
{
    sched->tasks = tasks;
    sched->tasks_number = tasks_number;
    sched->max_exec_time = 0;
    sched->run_count = 0;

    // maybe add check for null pointers for functions in each task
}

void sched_tick(scheduler_t *sched)
{
    // Read clock (maybe pass from outside so that I can test more easily)
    // Check runnable tasks (if nothing due, return immediately)
    // Run (timed) (one task per tick)
    // Update stats (run_count, max_exec_time)
    // Reschedule
}

task_t sched_choose_next_task(scheduler_t *sched)
{
    // Define a priority, if priority is a tie get the one that has waited more
}

// Think about how to change flight phase without breaking the layered structure
// eventually a function to change handle the phase change could be useful
// To handle correctly phase changes while a sensor is working I could make it look to it's activation status only when idle,
// so if it started measures, it could just finish it and return idle without the need of cleanup logic.
void sched_change_enabled(uint8_t *enabled_tasks)
{
    // Update one by one the on_enable / on_disable functions of each task and update the enabled_tasks array in the struct.
}

const task_t *sched_task(const scheduler_t *sched, uint8_t i)
{
    // Should simply get the task at the position specified,
}
