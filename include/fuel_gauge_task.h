#ifndef FUEL_GAUGE_TASK_H
#define FUEL_GAUGE_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t fuel_gauge_task_run(task_t *self, uint32_t now_millis);
    void fuel_gauge_task_on_enable(task_t *self);
    void fuel_gauge_task_on_disable(task_t *self);

    const task_t FUEL_GAUGE_TASK = {
        .run = &fuel_gauge_task_run,
        .on_enable = &fuel_gauge_task_on_enable,
        .on_disable = &fuel_gauge_task_on_disable,
        .last_state = TASK_STATE_OK,
        .name = "FUEL_GAUGE",
        .period = 20,
        .due = 0,
        .priority = 6,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif