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

// IDLE / ASCENT -> DESCENT: light must be seen continuously for this long
#define EJECTION_LIGHT_HOLD_MS 1000

// DESCENT -> LANDED: altitude and acceleration must stay stable for this long
#define LANDED_HOLD_MS 5000
#define LANDED_ALT_TOL_M 1.0f    /* max altitude drift from the start of the window */
#define LANDED_ACCEL_TOL_G 0.15f /* max distance of accel_mag from 1 g */

#ifdef __cplusplus
}
#endif

#endif