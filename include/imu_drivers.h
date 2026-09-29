#ifndef IMU_DRIVERS_H_
#define IMU_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    sens_external_state_t imu_wake(void);

    sens_external_state_t imu_poll(uint32_t now_ms);

    sens_external_state_t imu_get(imu_t *data);

    void imu_sleep(void);

#ifdef __cplusplus
}
#endif

#endif