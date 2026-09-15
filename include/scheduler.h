#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void sched_init(scheduler_t *sched, task_t *tasks, uint16_t tasks_number, uint8_t *enabled_tasks);
    void sched_tick(scheduler_t *sched);

#ifdef __cplusplus
}
#endif

#endif