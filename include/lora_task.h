#ifndef LORA_TASK_H
#define LORA_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t lora_task_run(task_t *self, uint32_t now_millis);
    void lora_task_on_enable(task_t *self);
    void lora_task_on_disable(task_t *self);

    const task_t SD_TASK_FAST = {
        .run = &lora_task_run,
        .on_enable = &lora_task_on_enable,
        .on_disable = &lora_task_on_disable,
        .last_state = TASK_STATE_OK,
        .name = "LORA",
        .period = 1500,
        .due = 0,
        .priority = 1,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif