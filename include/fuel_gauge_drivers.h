#ifndef FUEL_GAUGE_DRIVERS_H_
#define FUEL_GAUGE_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    sens_status_t fuel_gauge_sleep(void);

    // it recalculates battery calculations as if now were the first sample
    // could be useful after big power transitions

    void fuel_gauge_rebase_measures(void);
    // sets the sensor up and puts it in sleep mode. wait a little before calling wake
    sens_status_t fuel_gauge_init(void);

    sens_status_t fuel_gauge_wake();

    sens_status_t fuel_gauge_poll(uint32_t now_ms);

    sens_status_t fuel_gauge_get(battery_t *data);

#ifdef __cplusplus
}
#endif

#endif