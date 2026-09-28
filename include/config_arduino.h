#ifndef CONFIG_ARDUINO_H
#define CONFIG_ARDUINO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "arduino_wrapper.h"

/* PINS */
#define D_IN_PHOTORESISTOR WRAPPER_D2

#define D_OUT_GAS_COMMAND WRAPPER_D3

#define PWM_OUT_BLUE WRAPPER_D4
#define PWM_OUT_GREEN WRAPPER_D5
#define PWM_OUT_RED WRAPPER_D6

#define CHIPSELECT_SD WRAPPER_D7

#define DIO_LORA WRAPPER_D8
#define CHIPSELECT_LORA WRAPPER_D9
#define RESET_LORA WRAPPER_D10

#define A_IN_UV WRAPPER_D14

#define A_IN_GAS WRAPPER_D15

#define D_OUT_FPV_COMMAND WRAPPER_D16

#define D_OUT_GPS_COMMAND WRAPPER_D17

#ifdef __cplusplus
}
#endif

#endif