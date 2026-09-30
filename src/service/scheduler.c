#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "types.h"
#include "hal_time.h"
#include "config.h"

void sched_init(scheduler_t *sched, task_t *tasks, uint16_t tasks_number, bool *enabled_tasks);
void sched_tick(scheduler_t *sched);
static task_t *sched_choose_next_task(scheduler_t *sched, uint32_t now_millis, size_t *out_index);
void sched_change_enabled_tasks(scheduler_t *sched, const bool *new_enabled);
void sched_change_enabled_i(scheduler_t *sched, bool new_enabled, size_t index);
static task_t *sched_get_task(scheduler_t *sched, size_t i);

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
    size_t task_to_run_index;
    task_t *task_to_run = sched_choose_next_task(sched, start_millis, &task_to_run_index);

    if (task_to_run == NULL)
        return;

    uint32_t reschedule_interval = task_to_run->run(task_to_run, start_millis);
    uint32_t after_task_millis = hal_millis();

    if (task_to_run->last_state == TASK_STATE_ERR && task_to_run->fail_count > SCHED_MAX_FAILS)
    {
        // Disables a task if it has more than SCHED_MAX_FAILS fails
        sched_change_enabled_i(sched, 0, task_to_run_index);
    }

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

static task_t *sched_choose_next_task(scheduler_t *sched, uint32_t now_millis, size_t *out_index)
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
                *out_index = i;
            }
        }
    }

    return task_to_run;
}

void sched_change_enabled_tasks(scheduler_t *sched, const bool *new_enabled)
{
    size_t n = sched->tasks_number;

    for (size_t i = 0; i < n; i++)
    {
        sched_change_enabled_i(sched, new_enabled[i], i);
    }
}

void sched_change_enabled_i(scheduler_t *sched, bool new_enabled, size_t index)
{
    bool enabled_i = sched->enabled_tasks[index];

    if (enabled_i != new_enabled)
    {
        task_t *task = sched_get_task(sched, index);
        if (enabled_i == true) // hence new_enabled == false: the task is enabled and must be disabled
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
                task->on_disable(task);
        }
        else // enabled_i == false hence new_enabled = true: the task is disabled and must be enabled
        {
            if (task->on_enable != NULL)
                task->on_enable(task);
        }

        sched->enabled_tasks[index] = new_enabled;
    }
}

static task_t *sched_get_task(scheduler_t *sched, size_t i)
{
    if (i >= sched->tasks_number)
        return NULL;

    return sched->tasks + i;
}
