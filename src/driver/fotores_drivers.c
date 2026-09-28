#include <stdint.h>
#include "types.h"
#include "config.h"
#include "arduino_wrapper.h"

const uint32_t FOTORES_WAIT_TIME_MS = 2;

static sens_state_t state = SENS_IDLE;
static uint32_t last_sample_time = 0;
static fotores_t current_sample;

void fotores_init(void)
{
    wrapper_pin_mode(D_IN_PHOTORESISTOR, WRAPPER_INPUT);
    return;
}

sens_status_t fotores_poll(uint32_t now_ms)
{

    switch (state)
    {
    case SENS_IDLE:
        if ((now_ms - last_sample_time) < FOTORES_WAIT_TIME_MS)
            return SENS_NOT_READY;
        if (wrapper_digital_read(D_IN_PHOTORESISTOR) == WRAPPER_LOW)
        {
            current_sample.is_there_light = true;
            state = SENS_READY;
        }
        else
        {
            current_sample.is_there_light = false;
            state = SENS_READY;
        }
    case SENS_READY:
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_IDLE;
        return SENS_READY;
    }
    return SENS_ERR;
}

sens_status_t fotores_get(fotores_t *data)
{
    *data = current_sample;
    return SENS_NOT_READY;
}