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

#ifdef GROUND_STATION
/* Ground station board wiring (RFM95), see basestation/src/lora_config.h in
 * https://github.com/ApoTed/cansat-2026-stazione-radio */
#define DIO_LORA WRAPPER_D7
#define CHIPSELECT_LORA WRAPPER_D9
#define RESET_LORA WRAPPER_D8
/* DIO0 isn't wired on the ground board: lora_read_packet polls the radio's RxDone flag over SPI
 * instead of waiting for the interrupt. Remove it once IO0 is connected to D7 */
#define LORA_POLL_RX
#else
#define DIO_LORA WRAPPER_D8
#define CHIPSELECT_LORA WRAPPER_D9
#define RESET_LORA WRAPPER_D10
#endif

#define A_IN_UV WRAPPER_D14

#define A_IN_GAS WRAPPER_D15

#define D_OUT_FPV_COMMAND WRAPPER_D16

#define D_OUT_GPS_COMMAND WRAPPER_D17

#ifdef __cplusplus
}
#endif

#endif