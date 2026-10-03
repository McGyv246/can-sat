#include <stdint.h>
#include <Adafruit_MLX90640.h>
#include <Wire.h>
#include "types.h"
#include "I2C_wrapper_utils.h"

static sens_state_t state = SENS_STATE_NOT_READY;

static Adafruit_MLX90640 thermal;
static thermal_t current_sample;

// low range of the sensor (this will be blue on the screen)
static const uint8_t MINTEMP = 20;

// high range of the sensor (this will be red on the screen)
static const uint8_t MAXTEMP = 35;

extern "C"
{
    sens_state_t mlx_init(void)
    {
        if (!thermal.begin(MLX90640_I2CADDR_DEFAULT, &Wire))
            return SENS_STATE_ERR;
        thermal.setMode(MLX90640_CHESS);
        thermal.setResolution(MLX90640_ADC_17BIT);
        thermal.setRefreshRate(MLX90640_2_HZ); // 2 frames every second or so
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }

    sens_state_t mlx_poll(uint32_t now_ms)
    {
        if (state = SENS_STATE_NOT_READY)
            return state;
        uint16_t subpage_status;
        if ((read_reg_16_big_endian(MLX90640_I2CADDR_DEFAULT, 0x8000, &subpage_status) != SENS_STATE_ERR) && ((subpage_status & 0x0008) != 0))
            return SENS_STATE_NOT_READY;
        if (thermal.getFrame(current_sample.pixels) != 0)
            return SENS_STATE_ERR;
        state = SENS_STATE_READY;
        return SENS_STATE_READY;
    }

    void mlx_get(thermal_t *data)
    {
        if (state != SENS_STATE_READY)
            return SENS_STATE_ERR;
        *data = current_sample;
        state = SENS_STATE_IDLE;
        return;
    }
}