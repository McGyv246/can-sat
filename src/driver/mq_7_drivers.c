#include <stdint.h>
#include "arduino_wrapper.h"
#include "types.h"
#include "config.h"

const uint32_t MQ_7_WAIT_TIME_MS = 2;

static sens_state_t state = SENS_IDLE;
static uint32_t heater_start_time = 0; // instant the last heater on event happened
static gas_t current_sample;
static uint32_t last_sample_time = 0;

void mq_7_init(void)
{
    wrapper_pin_mode(D_OUT_GAS_COMMAND, WRAPPER_OUTPUT);
    wrapper_digital_write(D_OUT_GAS_COMMAND, WRAPPER_LOW);
    wrapper_pin_mode(A_IN_GAS, WRAPPER_INPUT);
    return;
}

void mq_7_wake(uint32_t now_ms)
{
    wrapper_digital_write(D_OUT_GAS_COMMAND, WRAPPER_HIGH);
    heater_start_time = now_ms;
    return;
}

sens_status_t mq_7_poll(uint32_t now_ms)
{
    switch (state)
    {
    case SENS_IDLE:
        if ((now_ms - last_sample_time) < MQ_7_WAIT_TIME_MS)
            return SENS_NOT_READY;
        current_sample.heater_enabled = true;
        current_sample.heater_on_s = (uint16_t)((now_ms - heater_start_time) / (1000));
        current_sample.raw = wrapper_analog_read(A_IN_GAS);
        state = SENS_READY;
    case SENS_READY:
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_IDLE;
        return SENS_READY;
    }
    return SENS_ERR;
}

sens_status_t mq_7_get(gas_t *data)
{
    *data = current_sample;
    return SENS_NOT_READY;
}

void mq_7_sleep(void)
{
    wrapper_digital_write(D_OUT_GAS_COMMAND, WRAPPER_LOW);
    return;
}