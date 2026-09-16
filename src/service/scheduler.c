#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "hal_time.h"

// TODO: Eventually move this
#define RESCHEDULE_FAULT_INTERVAL 1000

void sched_init(scheduler_t *sched, task_t *tasks, uint16_t tasks_number, bool *enabled_tasks);
void sched_tick(scheduler_t *sched);
task_t *sched_choose_next_task(scheduler_t *sched, uint32_t now_millis);
void sched_change_enabled_tasks(scheduler_t *sched, bool *new_enabled);
task_t *sched_get_task(scheduler_t *sched, size_t i);

void sched_init(scheduler_t *sched, task_t *tasks, uint16_t tasks_number, bool *enabled_tasks)
{
    sched->tasks = tasks;
    sched->tasks_number = tasks_number;
    sched->max_exec_time = 0;
    sched->run_count = 0;
    sched->enabled_tasks = enabled_tasks;
}

void sched_tick(scheduler_t *sched)
{
    uint32_t start_millis = hal_millis();
    task_t *task_to_run = sched_choose_next_task(sched, start_millis);

    if (task_to_run == NULL)
        return;

    uint32_t reschedule_interval = task_to_run->run();
    uint32_t after_task_millis = hal_millis();

    sched->run_count += 1;
    if (after_task_millis - start_millis > sched->max_exec_time)
    {
        sched->max_exec_time = after_task_millis - start_millis;
    }

    task_to_run->due += reschedule_interval;

    if (task_to_run->due < after_task_millis - RESCHEDULE_FAULT_INTERVAL)
    {
        task_to_run->due = after_task_millis + reschedule_interval;
    }
}

task_t *sched_choose_next_task(scheduler_t *sched, uint32_t now_millis)
{
    task_t *task_to_run = NULL;

    for (size_t i = 0; i < sched->tasks_number; i++)
    {
        task_t *current_task = sched_get_task(sched, i);
        if (current_task->due <= now_millis && sched->enabled_tasks[i])
        {
            // TODO: eventually add priority for tasks whose due time is farther
            if ((task_to_run != NULL && task_to_run->priority > current_task->priority) || task_to_run == NULL)
            {
                task_to_run = current_task;
            }
        }
    }

    return task_to_run;
}

void sched_change_enabled_tasks(scheduler_t *sched, bool *new_enabled)
{
    bool *current_enabled = sched->enabled_tasks;
    size_t n = sched->tasks_number;

    for (size_t i = 0; i < n; i++)
    {
        bool enabled_i = current_enabled[i];
        bool new_i = new_enabled[i];

        if (enabled_i != new_i)
        {
            task_t *task = sched_get_task(sched, i);
            if (enabled_i == true) // hence new_i == false: the task is enabled and must be disabled
            {
                /*
                TODO: if necessary here I could check if the task is idle or not:
                if idle I could just disable it
                if not idle I could postpone the inactivation.
                Since this function would be called from outside I this file and
                the caller doesn't want to handle if one task has not been disabled because idle,
                I could activate a special task that checks every a few passes if the missing
                tasks became idle and can be deactivated.
                For simplicity and time for now it will remain as is.
                */
                if (task->on_disable != NULL)
                    task->on_disable();
            }
            else // enabled_i == false hence new_i = true: the task is disabled and must be enabled
            {
                if (task->on_enable != NULL)
                    task->on_enable();
            }

            // Made the array immutable by assigning directly (below)
            // if it becomes necessary modify the array directly
            // current_enabled[i] = new_enabled[i];
        }
    }

    sched->enabled_tasks = new_enabled;
}

task_t *sched_get_task(scheduler_t *sched, size_t i)
{
    if (i >= sched->tasks_number)
        return NULL;

    return sched->tasks + i;
}
