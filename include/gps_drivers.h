#ifndef GPS_DRIVERS_H
#define GPS_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    // this function starts the gps up to let it find satellites. it can be recaled to check on the sensor.
    // if satellites have been found, SENS_STATE_READY will be returned. SENS_STATE_NOT_READY will be returned otherwise
    // note: final checks chould be the led light on the gps module
    sens_state_t gps_init(void);

    sens_state_t gps_wake(void);

    sens_state_t gps_poll(uint32_t now_ms);

    void gps_get(gps_t *data);

    sens_state_t gps_sleep(void);

#define GPS_DEBUG_MAX_SATS 16

    /*
    Diagnostics, to understand why the GPS doesn't lock. Filled while parsing the NMEA sentences,
    only meaningful while the GPS is on. Counters are since boot.
    */
    typedef struct
    {
        uint32_t chars;           /* bytes parsed: if it doesn't grow, no data arrives (wiring, power, baud) */
        uint32_t passed_checksum; /* good sentences */
        uint32_t failed_checksum; /* corrupted sentences: noise, wrong baud or bytes lost */
        uint32_t rx_overflows;    /* reads that found the RX FIFO overflowed (bytes lost, polled too late) */
        uint32_t boots;           /* u-blox boot banners seen: each one is a restart of the module (power dip?) */
        char antenna[16];         /* last "ANTSTATUS=..." text of the module, empty if never sent */
        uint8_t fix_mode;         /* from GSA: 1 no fix, 2 2D, 3 3D, 0 not received yet */
        uint8_t sats_in_view;     /* from GSV: satellites the almanac says are above the horizon */
        uint8_t sats_count;       /* entries in prn / snr, from the last complete GSV cycle */
        uint8_t prn[GPS_DEBUG_MAX_SATS];
        uint8_t snr[GPS_DEBUG_MAX_SATS]; /* C/N0 in dB-Hz, 0 = not tracked. ~25+ is needed to lock */
        bool time_valid;                 /* UTC time known: at least one satellite was decoded */
        uint32_t on_ms;                  /* ms since the last power on */
    } gps_debug_t;

    void gps_get_debug(gps_debug_t *d);

#ifdef __cplusplus
}
#endif

#endif