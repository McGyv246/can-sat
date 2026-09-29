#ifndef GYBMEP_DRIVERS_H_
#define GYBMEP_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    // checks if the address and chip_id of the sensor are correct. this function is to be called once
    // to check everything is in order, the sensor is then initialized in sleep mode
    sens_external_state_t gybmep_init(void);

    sens_external_state_t gybmep_wake(void);

    sens_external_state_t gybmep_poll(uint32_t now_ms);

    sens_external_state_t gybmep_get(env_t *data);

    // if a measure is currently being taken by the sensor it will have to finish it before going to sleep
    // it shouldn't be a problem here: once this function is called the configuration registers are written
    // and once the measure is over, the sensor should autonomously go back to sleep
    // leaving this here as a note in case i fucked up
    sens_external_state_t gybmep_sleep(void);

#ifdef __cplusplus
}
#endif

#endif