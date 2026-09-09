#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* PINS */
#define D_IN_PHOTORESISTOR 5

#define D_OUT_GAS_COMMAND 6

#define PWM_OUT_BLUE 7
#define PWM_OUT_GREEN 8
#define PWM_OUT_RED 9

#define CHIPSELECT_SD 10

#define DIO_LORA 11
#define CHIPSELECT_LORA 12
#define RESET_LORA 13

#define A_IN_UV 19

// Still need to choose which sensors to use, will change names when chosen
#define A_IN_GAS1 20
#define A_IN_GAS2 21

    // SAMPLE RATES (to be defined)

    // THRESHOLDS (such as ones to be used to determine flight state) (to be defined)

#ifdef __cplusplus
}
#endif

#endif