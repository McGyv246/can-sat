#include <stdint.h>
#include <Adafruit_MLX90640.h>
#include <Wire.h>
#include "types.h"
#include "I2C_wrapper_utils.h"

// MLX90640 status register: bit 3 is set when a new subpage has been measured
static const uint16_t STATUS_REGISTER = 0x8000;
static const uint16_t STATUS_NEW_DATA = 0x0008;

static sens_state_t state = SENS_STATE_NOT_READY; // until mlx_init succeeds

static Adafruit_MLX90640 thermal;
static thermal_t current_sample;

extern "C"
{
    sens_state_t mlx_init(void)
    {
        if (!thermal.begin(MLX90640_I2CADDR_DEFAULT, &Wire))
            return SENS_STATE_ERR;
        thermal.setMode(MLX90640_CHESS);
        thermal.setResolution(MLX90640_ADC_17BIT);
        thermal.setRefreshRate(MLX90640_2_HZ); // 2 subpages per second: a full frame (both subpages) every second
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }

    sens_state_t mlx_poll(uint32_t now_ms)
    {
        if (state == SENS_STATE_NOT_READY)
            return SENS_STATE_ERR; // never initialized, or mlx_init failed
        uint16_t status;
        if (read_reg_16_big_endian(MLX90640_I2CADDR_DEFAULT, STATUS_REGISTER, &status) == SENS_STATE_ERR)
            return SENS_STATE_ERR;
        if ((status & STATUS_NEW_DATA) == 0)
            return SENS_STATE_NOT_READY;
        // blocks until the other subpage is ready too (up to one subpage period, 500 ms at 2 Hz)
        if (thermal.getFrame(current_sample.pixels) != 0)
            return SENS_STATE_ERR;
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_STATE_READY;
        return SENS_STATE_READY;
    }

    sens_state_t mlx_get(thermal_t *data)
    {
        if (state != SENS_STATE_READY)
            return SENS_STATE_ERR;
        *data = current_sample;
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }
}