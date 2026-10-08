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
        int8_t elev[GPS_DEBUG_MAX_SATS]; /* elevation in degrees, -1 = empty: no orbit (almanac/ephemeris) for it yet */
        bool time_valid;                 /* UTC time known: decoded from a satellite, or kept by the module's RTC */
        uint32_t on_ms;                  /* ms since the last power on */

        /* From the UBX-MON-HW replies to gps_request_hw_status (interference measured by the module) */
        uint32_t hw_requests;   /* MON-HW polls sent since the last power on */
        uint32_t hw_replies;    /* MON-HW replies received since the last power on: 0 = the module doesn't answer */
        uint16_t noise_per_ms;  /* noise level measured by the module */
        uint16_t agc_cnt;       /* AGC monitor, 0-8191: how much the front end amplifies the input */
        uint8_t jam_ind;        /* CW jamming indicator, 0 = none, 255 = strong */
        uint8_t jam_state;      /* jamming monitor: 0 unknown/disabled, 1 ok, 2 warning, 3 critical */
        uint8_t itfm_ack;       /* jamming monitor enable (CFG-ITFM): 0 not sent, 1 no answer, 2 ACK, 3 NAK */
    } gps_debug_t;

    void gps_get_debug(gps_debug_t *d);

    // Asks the module for its hardware status (UBX-MON-HW): the reply fills the hw fields of gps_debug_t
    // when it arrives (within ~1 s). Until the module answers it, every call also enables the jamming monitor
    // (UBX-CFG-ITFM, in the module RAM only: lost at power off). Only for debugging, does nothing if off
    void gps_request_hw_status(void);

    // Cold start (UBX-CFG-RST): the module erases ephemeris, almanac, last position and time from its
    // backup memory and searches again from nothing. The settings are kept. on_ms restarts from 0, so the
    // debug print shows the time to the first fix. Returns false if the GPS is off (nothing sent)
    bool gps_cold_start(void);

    // When on, every NMEA line received is printed on Serial with DBG_INFO (needs SERIAL_DEBUG_INFO). Off at boot
    void gps_set_raw_echo(bool on);

#ifdef __cplusplus
}
#endif

#endif