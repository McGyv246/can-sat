#ifndef GPS_TASK_H
#define GPS_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t gps_task_run(task_t *self, uint32_t now_millis);
    void gps_task_on_enable(task_t *self);
    void gps_task_on_disable(task_t *self);

    const task_t GPS_TASK = {
        .run = &gps_task_run,
        .on_enable = &gps_task_on_enable,
        .on_disable = &gps_task_on_disable,
        .last_state = TASK_STATE_OK,
        .name = "GPS",
        .period = 500,
        .due = 0,
        .priority = 0,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif
