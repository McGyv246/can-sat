#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#define RESCHEDULE_FAULT_INTERVAL 1000

#define SCHED_MAX_FAILS 20

// Must be power of 2 and multiple of 512
#ifndef BUF_SIZE
// This if is necessary so that I can override this macro in tests
// by setting it in the compile command.
#define BUF_SIZE 32768
#endif

#define SYNC_WORD 0xA5A5

// IDLE -> ASCENT: altitude must stay this far above the ground (lowest altitude seen in IDLE) for this long
#define ASCENT_ALT_DELTA_M 15.0f
#define ASCENT_HOLD_MS 1000

// IDLE / ASCENT -> DESCENT: light must be seen continuously for this long
#define EJECTION_LIGHT_HOLD_MS 1000

// IDLE only: the light check is armed after it has been dark continuously for this long (rocket closed),
// so the light seen while the CanSat is turned on with the rocket open doesn't trigger the ejection
#define LIGHT_ARM_DARK_HOLD_MS 10000

// DESCENT -> LANDED: altitude and acceleration must stay stable for this long
#define LANDED_HOLD_MS 5000
#define LANDED_ALT_TOL_M 1.0f    /* max altitude drift from the start of the window */
#define LANDED_ACCEL_TOL_G 0.15f /* max distance of accel_mag from 1 g */

// Status led (see led_status.h)
#define LED_BATTERY_LOW_PCT 10.0f /* same as the fuel gauge alert threshold */

// GPS warm up in IDLE (see gps_warmup.h)
#define GPS_POWER_ON_DELAY_MS 10000UL  /* GPS off for this long after IDLE starts: powering it on at boot browns out the board */
#define GPS_LOCK_HOLD_MS 60000UL       /* valid fix kept this long before turning it off */
#define GPS_REFRESH_MS 3600000UL       /* turned on again this long after the last lock to refresh the ephemeris */
#define GPS_WARM_VALID_MS 7200000UL    /* a lock older than this is too old for a hot start */

#ifdef __cplusplus
}
#endif

#endif