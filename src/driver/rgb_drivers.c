#include "arduino_wrapper.h"
#include "config.h"
#include "config_arduino.h"
#include "types.h"

void rgb_sleep(void)
{
    wrapper_digital_write(PWM_OUT_RED, WRAPPER_LOW);
    wrapper_digital_write(PWM_OUT_GREEN, WRAPPER_LOW);
    wrapper_digital_write(PWM_OUT_BLUE, WRAPPER_LOW);
    return;
}

void rgb_init(void)
{
    wrapper_pin_mode(PWM_OUT_RED, WRAPPER_OUTPUT);
    wrapper_pin_mode(PWM_OUT_GREEN, WRAPPER_OUTPUT);
    wrapper_pin_mode(PWM_OUT_BLUE, WRAPPER_OUTPUT);
    rgb_sleep();
    return;
}

void rgb_set_color(rgb_colors_t color)
{
    wrapper_analog_write(PWM_OUT_RED, color.red);
    wrapper_analog_write(PWM_OUT_GREEN, color.green);
    wrapper_analog_write(PWM_OUT_BLUE, color.blue);
    return;
}