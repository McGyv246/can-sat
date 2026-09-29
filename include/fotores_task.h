#ifndef FOTORES_TASK_H
#define FOTORES_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t fotores_task_run(task_t *self, uint32_t now_millis);

    const task_t FOTORES_TASK_FAST = {
        .run = &fotores_task_run,
        .on_enable = NULL,
        .on_disable = NULL,
        .last_state = TASK_STATE_OK,
        .name = "FOTORES",
        .period = 20,
        .due = 0,
        .priority = 4,
        .fail_count = 0,
    };

    const task_t FOTORES_TASK_SLOW = {
        .run = &fotores_task_run,
        .on_enable = NULL,
        .on_disable = NULL,
        .last_state = TASK_STATE_OK,
        .name = "FOTORES",
        .period = 1000,
        .due = 0,
        .priority = 4,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif