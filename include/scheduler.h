#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void sched_init(scheduler_t *sched, task_t *tasks, uint16_t tasks_number, bool *enabled_tasks);
    void sched_tick(scheduler_t *sched);
    void sched_change_enabled_tasks(scheduler_t *sched, const bool *new_enabled);
    void sched_change_enabled_i(scheduler_t *sched, bool new_enabled, size_t index);

#ifdef __cplusplus
}
#endif

#endif