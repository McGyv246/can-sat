#include "arduino_wrapper.h"
#include "types.h"
#include "config.h"
#include "config_arduino.h"

static const uint32_t UV_WAIT_TIME_MS = 2;
static const float VCC = 3.3;
static const uint8_t ADC_BITS = 12;
static const uint16_t MAX_V = 4095; // (1 << ADC_BITS) - 1
static const float ADC_CONVERSION_FACTOR = VCC / (float)MAX_V;
static const float VOLTAGE_INDEX_FACTOR = 0.1;
static const float UV_TRESH = 2;

static sens_state_t state = SENS_STATE_IDLE;
static uint32_t last_sample_time = 0;
static uv_t latest_sample;

void uv_init()
{
    wrapper_pin_mode(A_IN_UV, WRAPPER_INPUT);
    // without this analogRead gives 0-1023 and the index comes out 4 times too low
    wrapper_analog_read_resolution(ADC_BITS);
    return;
}

sens_state_t uv_poll(uint32_t now_ms)
{

    switch (state)
    {
    case SENS_STATE_IDLE:
        if (now_ms - last_sample_time < UV_WAIT_TIME_MS)
            return SENS_STATE_NOT_READY;
        latest_sample.uv_raw = wrapper_analog_read(A_IN_UV);
        latest_sample.uv_index = (float)(latest_sample.uv_raw * ADC_CONVERSION_FACTOR) / VOLTAGE_INDEX_FACTOR;
        if (latest_sample.uv_index > UV_TRESH)
        {
            latest_sample.is_there_light = true;
        }
        else
            latest_sample.is_there_light = false;
        latest_sample.s.valid = true;
        latest_sample.s.t_ms = now_ms;
        last_sample_time = now_ms;
        state = SENS_STATE_READY;

    case SENS_STATE_READY:
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }
    return SENS_STATE_ERR;
}

void uv_get(uv_t *data)
{
    *data = latest_sample;
    return;
}
