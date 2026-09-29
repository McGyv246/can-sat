#include <stdint.h>
#include <Arduino_LSM6DSOX.h>
#include "types.h"

static sens_internal_state_t state = SENS_INTERNAL_IDLE;
static imu_t current_sample;

sens_external_state_t imu_wake(void)
{
    if (!IMU.begin())
        return SENS_EXTERNAL_ERR;
    return SENS_EXTERNAL_NOT_READY;
}

sens_external_state_t imu_poll(uint32_t now_ms)
{
    switch (state)
    {
    case SENS_INTERNAL_IDLE:
        if (!IMU.accelerationAvailable() || !IMU.gyroscopeAvailable())
        {
            state = SENS_INTERNAL_WAITING;
            return SENS_EXTERNAL_NOT_READY;
        }
        state = SENS_INTERNAL_OK;
    case SENS_INTERNAL_WAITING:
        if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
        {
            state = SENS_INTERNAL_OK;
        }
        else
            return SENS_EXTERNAL_NOT_READY;
    case SENS_INTERNAL_OK:
        if (!IMU.readAcceleration(current_sample.accel[0], current_sample.accel[1], current_sample.accel[2]))
            return SENS_EXTERNAL_ERR;
        if (!IMU.readGyroscope(current_sample.gyro[0], current_sample.gyro[1], current_sample.gyro[2]))
            return SENS_EXTERNAL_ERR;
        current_sample.accel_mag = sqrtf((current_sample.accel[0] * current_sample.accel[0]) +
                                         (current_sample.accel[1] * current_sample.accel[1]) +
                                         (current_sample.accel[2] * current_sample.accel[2]));
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_INTERNAL_IDLE;
        return SENS_EXTERNAL_READY;
    }
    return SENS_EXTERNAL_ERR;
}

sens_external_state_t imu_get(imu_t *data)
{
    *data = current_sample;
    return SENS_EXTERNAL_NOT_READY;
}

void imu_sleep(void)
{
    IMU.end();
}
