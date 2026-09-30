#ifndef SD_TASK_H
#define SD_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t sd_task_run(task_t *self, uint32_t now_millis);

    const task_t SD_TASK_FAST = {
        .run = &sd_task_run,
        .on_enable = NULL,
        .on_disable = NULL,
        .last_state = TASK_STATE_OK,
        .name = "SD",
        .period = 300,
        .due = 0,
        .priority = 0,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif