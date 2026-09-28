#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <arduino_wrapper.h>

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

/* OTHER CONSTANTS */
#define RESCHEDULE_FAULT_INTERVAL 1000

// Must be power of 2 and multiple of 512
#ifndef BUF_SIZE
// This if is necessary so that I can override this macro in tests
// by setting it in the compile command.
#define BUF_SIZE 32768
#endif

#define SYNC_WORD 0xA5A5

    // SAMPLE RATES (to be defined)

    // THRESHOLDS (such as ones to be used to determine flight state) (to be defined)

#ifdef __cplusplus
}
#endif

#endif