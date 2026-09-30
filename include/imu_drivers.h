#ifndef IMU_DRIVERS_H
#define IMU_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    sens_state_t imu_wake(void);

    sens_state_t imu_poll(uint32_t now_ms);

    void imu_get(imu_t *data);

    sens_state_t imu_sleep(void);

#ifdef __cplusplus
}
#endif

#endif