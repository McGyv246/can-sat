#ifndef GPS_DRIVERS_H_
#define GGPS_DRIVERS_H_

#include <stdint.h>
#include "types.h"

// this function starts the gps up to let it find satellites. it can be recaled to check on the sensor.
// if satellites have been found, SENS_READY will be returned. SENS_NOT_READY will be returned otherwise
// note: final checks chould be the led light on the gps module
sens_status_t gps_init(void);

sens_status_t gps_wake(void);

sens_status_t gps_poll(uint32_t now_ms);

sens_status_t gps_get(gps_t *data);

sens_status_t gps_sleep(void);

#endif