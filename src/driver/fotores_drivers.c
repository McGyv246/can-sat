#include <stdint.h>
#include "types.h"
#include "config.h"
#include "config_arduino.h"
#include "arduino_wrapper.h"

static const uint32_t FOTORES_WAIT_TIME_MS = 2;

static sens_state_t state = SENS_STATE_IDLE;
static uint32_t last_sample_time = 0;
static fotores_t current_sample;

void fotores_init(void)
{
    wrapper_pin_mode(D_IN_PHOTORESISTOR, WRAPPER_INPUT);
    return;
}

sens_state_t fotores_poll(uint32_t now_ms)
{

    switch (state)
    {
    case SENS_STATE_IDLE:
        if ((now_ms - last_sample_time) < FOTORES_WAIT_TIME_MS)
            return SENS_STATE_NOT_READY;
        last_sample_time = now_ms;
        if (wrapper_digital_read(D_IN_PHOTORESISTOR) == WRAPPER_LOW)
        {
            current_sample.is_there_light = true;
            state = SENS_STATE_READY;
        }
        else
        {
            current_sample.is_there_light = false;
            state = SENS_STATE_READY;
        }
    case SENS_STATE_READY:
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }
    return SENS_STATE_ERR;
}

void fotores_get(fotores_t *data)
{
    *data = current_sample;
    return;
}