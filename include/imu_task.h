#ifndef IMU_TASK_H
#define IMU_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    uint32_t imu_task_run(task_t *self, uint32_t now_millis);
    void imu_task_on_enable(task_t *self);
    void imu_task_on_disable(task_t *self);

    const task_t IMU_TASK = {
        .run = &imu_task_run,
        .on_enable = &imu_task_on_enable,
        .on_disable = &imu_task_on_disable,
        .last_state = TASK_STATE_OK,
        .name = "IMU",
        .period = 500,
        .due = 0,
        .priority = 1,
        .fail_count = 0,
    };

#ifdef __cplusplus
}
#endif

#endif