#include <stdint.h>
#include "arduino_wrapper.h"
#include "types.h"
#include "config.h"
#include "config_arduino.h"

static const uint32_t MQ_7_WAIT_TIME_MS = 2;
// same resolution the UV driver sets: the setting is shared by all analog pins, so raw is 0-4095
static const uint8_t ADC_BITS = 12;

static sens_state_t state = SENS_STATE_IDLE;
static uint32_t heater_start_time = 0; // instant the last heater on event happened
static gas_t current_sample;
static uint32_t last_sample_time = 0;

void mq_7_init(void)
{
    wrapper_pin_mode(D_OUT_GAS_COMMAND, WRAPPER_OUTPUT);
    wrapper_digital_write(D_OUT_GAS_COMMAND, WRAPPER_LOW);
    wrapper_pin_mode(A_IN_GAS, WRAPPER_INPUT);
    wrapper_analog_read_resolution(ADC_BITS);
    current_sample.heater_enabled = false;
    return;
}

void mq_7_wake(uint32_t now_ms)
{
    wrapper_digital_write(D_OUT_GAS_COMMAND, WRAPPER_HIGH);
    heater_start_time = now_ms;
    return;
}

sens_state_t mq_7_poll(uint32_t now_ms)
{
    switch (state)
    {
    case SENS_STATE_IDLE:
        if ((now_ms - last_sample_time) < MQ_7_WAIT_TIME_MS)
            return SENS_STATE_NOT_READY;
        last_sample_time = now_ms;
        current_sample.heater_enabled = true;
        current_sample.heater_on_s = (uint16_t)((now_ms - heater_start_time) / (1000));
        current_sample.raw = wrapper_analog_read(A_IN_GAS);
        state = SENS_STATE_READY;
    case SENS_STATE_READY:
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }
    return SENS_STATE_ERR;
}

void mq_7_get(gas_t *data)
{
    *data = current_sample;
    return;
}

void mq_7_sleep(void)
{
    wrapper_digital_write(D_OUT_GAS_COMMAND, WRAPPER_LOW);
    current_sample.heater_enabled = false;
    state = SENS_STATE_IDLE;
    return;
}