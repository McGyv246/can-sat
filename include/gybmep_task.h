#ifndef GYBMEP_TASK_H
#define GYBMEP_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t gybmep_task_run(task_t *self, uint32_t now_millis);
    void gybmep_task_on_enable(task_t *self);
    void gybmep_task_on_disable(task_t *self);

    const task_t GYBMEP_TASK = {
        .run = &gybmep_task_run,
        .on_enable = &gybmep_task_on_enable,
        .on_disable = &gybmep_task_on_disable,
        .last_state = TASK_STATE_OK,
        .name = "GYBMEP",
        .period = 500,
        .due = 0,
        .priority = 2,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif