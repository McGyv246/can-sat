#include <stdint.h>
#include <Arduino_LSM6DSOX.h>
#include "types.h"
#include "imu_drivers.h"

static sens_state_t state = SENS_STATE_IDLE;
static imu_t current_sample;

extern "C"
{
    sens_state_t imu_wake(void)
    {
        if (!IMU.begin())
            return SENS_STATE_ERR;
        return SENS_STATE_READY;
    }

    sens_state_t imu_poll(uint32_t now_ms)
    {
        switch (state)
        {
        case SENS_STATE_IDLE:
            if (!IMU.accelerationAvailable() || !IMU.gyroscopeAvailable())
            {
                state = SENS_STATE_NOT_READY;
                return SENS_STATE_NOT_READY;
            }
            state = SENS_STATE_READY;
        case SENS_STATE_NOT_READY:
            if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable())
            {
                state = SENS_STATE_READY;
            }
            else
                return SENS_STATE_NOT_READY;
        case SENS_STATE_READY:
            if (!IMU.readAcceleration(current_sample.accel[0], current_sample.accel[1], current_sample.accel[2]))
                return SENS_STATE_ERR;
            if (!IMU.readGyroscope(current_sample.gyro[0], current_sample.gyro[1], current_sample.gyro[2]))
                return SENS_STATE_ERR;
            current_sample.accel_mag = sqrtf((current_sample.accel[0] * current_sample.accel[0]) +
                                             (current_sample.accel[1] * current_sample.accel[1]) +
                                             (current_sample.accel[2] * current_sample.accel[2]));
            current_sample.s.t_ms = now_ms;
            current_sample.s.valid = true;
            state = SENS_STATE_IDLE;
            return SENS_STATE_READY;
        }
        return SENS_STATE_ERR;
    }

    void imu_get(imu_t *data)
    {
        *data = current_sample;
        return;
    }

    sens_state_t imu_sleep(void)
    {
        IMU.end();
        return SENS_STATE_NOT_READY;
    }
}
