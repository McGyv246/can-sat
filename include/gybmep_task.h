#ifndef GYBMEP_TASK_H
#define GYBMEP_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t run(task_t *self, uint32_t now_millis);
    void on_enable(task_t *self);
    void on_disable(task_t *self);

    const task_t GYBMEP_TASK = {
        .run = &run,
        .on_enable = &on_enable,
        .on_disable = &on_disable,
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