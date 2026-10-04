#include <stdint.h>
#include <Arduino_LSM6DSOX.h>
#include "types.h"
#include "imu_drivers.h"
#include "wire_wrapper.h"
#include "I2C_wrapper_utils.h"
#include "debug_print.h"

/*
The LSM6DSOX is on the same I2C bus as the other sensors (GYBMEP, fuel gauge), but the library's
IMU.end() calls Wire.end(), turning the bus off for everyone. IMU.begin() does it too when the chip
doesn't answer. So IMU.end() is never called here: sleep writes the power down registers directly.
*/
static const uint8_t ADDRESS = 0x6A;
static const uint8_t REG_CTRL1_XL = 0x10; // accelerometer ODR, 0 = power down
static const uint8_t REG_CTRL2_G = 0x11;  // gyroscope ODR, 0 = power down

static sens_state_t state = SENS_STATE_IDLE;
static imu_t current_sample;

extern "C"
{
    sens_state_t imu_wake(void)
    {
        state = SENS_STATE_IDLE;
        if (!IMU.begin())
        {
            wire_begin_service(); // begin() failed and turned the bus off: turn it back on for the other sensors
            DBG_ERR("IMU LSM6DSOX not answering on 0x%02X", ADDRESS);
            return SENS_STATE_ERR;
        }
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
        // same as IMU.end() without the Wire.end()
        if (write_reg_8(ADDRESS, REG_CTRL2_G, 0x00) == SENS_STATE_ERR)
            return SENS_STATE_ERR;
        if (write_reg_8(ADDRESS, REG_CTRL1_XL, 0x00) == SENS_STATE_ERR)
            return SENS_STATE_ERR;
        state = SENS_STATE_IDLE;
        return SENS_STATE_NOT_READY;
    }
}
