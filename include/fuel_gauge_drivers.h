#ifndef FUEL_GAUGE_DRIVERS_H
#define FUEL_GAUGE_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    sens_state_t fuel_gauge_sleep(void);

    // it recalculates battery calculations as if now were the first sample
    // could be useful after big power transitions

    void fuel_gauge_rebase_measures(void);
    // sets the sensor up, redoes the SOC estimate (rebase) and puts it in sleep mode. wait a little before calling wake.
    // call it before turning on big loads (GPS, MQ-7 heater): the estimate is only good with the battery at rest.
    // it blocks for ~750 ms
    sens_state_t fuel_gauge_init(void);

    sens_state_t fuel_gauge_wake();

    sens_state_t fuel_gauge_poll(uint32_t now_ms);

    void fuel_gauge_get(battery_t *data);

#ifdef __cplusplus
}
#endif

#endif