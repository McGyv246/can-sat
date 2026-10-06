#ifndef GPS_WARMUP_H
#define GPS_WARMUP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

    /*
    Decides when the GPS must be powered, so that it's ready for a fast fix in flight.

    The NEO-6M keeps the satellites it found (ephemeris, valid for ~2-4 h) in its backup RAM, powered by
    the cell on the module even when D17 cuts its power. With fresh ephemeris it gets a fix in a few seconds
    (hot start) instead of minutes. So in IDLE, on the launch site:
    - WAITING:   GPS off for GPS_POWER_ON_DELAY_MS, so its inrush doesn't add to the boot ones (brownout)
    - SEARCHING: GPS on until it keeps a valid fix for GPS_LOCK_HOLD_MS
    - ASLEEP:    GPS off, satellites saved (warm). After GPS_REFRESH_MS it goes back to SEARCHING
                 to refresh the ephemeris, in case IDLE lasts long
    In every other flight state the GPS is always on: in ASCENT it's turned on again and finds the
    saved satellites in a few seconds (the samples before the fix have valid = false).
    */
    typedef enum
    {
        GPS_WARMUP_WAITING,
        GPS_WARMUP_SEARCHING,
        GPS_WARMUP_ASLEEP,
    } gps_warmup_phase_t;

    typedef struct
    {
        gps_warmup_phase_t phase;
        uint32_t phase_since_ms;
        bool started;          /* false until the first update in IDLE */
        bool fix_active;       /* SEARCHING only: is there a valid fix right now? */
        uint32_t fix_since_ms; /* when the current valid fix started */
        bool warm;             /* the GPS has been locked at least once */
        uint32_t last_lock_ms; /* when the last lock completed (fix held for GPS_LOCK_HOLD_MS) */
    } gps_warmup_t;

    void gps_warmup_reset(gps_warmup_t *w);

    // Returns true if the GPS must be powered now. fix_valid is the valid field of the latest GPS sample
    bool gps_warmup_update(gps_warmup_t *w, flight_state_t state, bool fix_valid, uint32_t now_ms);

    // true if a lock completed in the last GPS_WARM_VALID_MS, so the saved satellites are good for a hot start
    bool gps_warmup_is_warm(const gps_warmup_t *w, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
