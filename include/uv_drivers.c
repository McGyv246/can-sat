#include <types.h>
#include <config.h>
#include <Arduino.h>

const uint32_t UV_WAIT_TIME_MS = 2;
const float VCC = 3.3;
const uint16_t MAX_V = 4095;
const float ADC_CONVERSION_FACTOR = VCC / (float)MAX_V;
const float VOLTAGE_INDEX_FACTOR = 0.1;
const float UV_TRESH = 2;

static sens_state_t state = SENS_IDLE;
static uint32_t last_sample_time;
static light_t latest_sample;

sens_status_t uv_poll(uint32_t now_ms)
{

    switch (state)
    {
    case SENS_IDLE:
        if (now_ms - last_sample_time < UV_WAIT_TIME_MS)
            return SENS_NOT_READY;
        latest_sample.uv_raw = analogRead(A_IN_UV);
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
        state = SENS_OK;

    case SENS_WAITING:
        return SENS_ERR;

    case SENS_OK:
        return SENS_READY;
    }
    return SENS_ERR;
}

sens_status_t uv_get(light_t *data)
{
    *data = latest_sample;
    return SENS_NOT_READY;
}
