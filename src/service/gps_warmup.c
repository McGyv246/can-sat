#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "types.h"
#include "gps_warmup.h"

static void enter_phase(gps_warmup_t *w, gps_warmup_phase_t phase, uint32_t now_ms)
{
    w->phase = phase;
    w->phase_since_ms = now_ms;
    w->fix_active = false;
}

void gps_warmup_reset(gps_warmup_t *w)
{
    *w = (gps_warmup_t){0};
}

bool gps_warmup_update(gps_warmup_t *w, flight_state_t state, bool fix_valid, uint32_t now_ms)
{
    if (state != FLIGHT_STATE_IDLE)
        return true;

    if (!w->started)
    {
        w->started = true;
        enter_phase(w, GPS_WARMUP_WAITING, now_ms);
    }

    switch (w->phase)
    {
    case GPS_WARMUP_WAITING:
        if ((now_ms - w->phase_since_ms) >= GPS_POWER_ON_DELAY_MS)
            enter_phase(w, GPS_WARMUP_SEARCHING, now_ms);
        break;

    case GPS_WARMUP_SEARCHING:
        if (!fix_valid)
        {
            w->fix_active = false;
            break;
        }
        if (!w->fix_active)
        {
            w->fix_active = true;
            w->fix_since_ms = now_ms;
        }
        // Each satellite sends its ephemeris every 30 s: holding the fix longer saves all the visible ones
        if ((now_ms - w->fix_since_ms) >= GPS_LOCK_HOLD_MS)
        {
            w->warm = true;
            w->last_lock_ms = now_ms;
            enter_phase(w, GPS_WARMUP_ASLEEP, now_ms);
        }
        break;

    case GPS_WARMUP_ASLEEP:
        if ((now_ms - w->phase_since_ms) >= GPS_REFRESH_MS)
            enter_phase(w, GPS_WARMUP_SEARCHING, now_ms);
        break;

    default:
        break;
    }

    return w->phase == GPS_WARMUP_SEARCHING;
}

bool gps_warmup_is_warm(const gps_warmup_t *w, uint32_t now_ms)
{
    return w->warm && (now_ms - w->last_lock_ms) <= GPS_WARM_VALID_MS;
}
