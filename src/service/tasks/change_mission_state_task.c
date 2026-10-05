#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "config.h"
#include "types.h"
#include "tasks.h"
#include "datastore.h"
#include "debug_print.h"

/*
Decides when the flight state changes and writes it in the datastore.
It doesn't touch the schedulers: each core reads the state from the datastore and
applies its own profile (see tasks_profile_core0/1 in tasks.c).

Since the datastore only holds the latest measure, conditions over a timespan are
checked with the persistence helpers below, which only remember when the condition
started being true instead of the whole history.
*/

#pragma region persistence_helpers

typedef struct
{
    uint32_t since_ms; /* when the condition became true */
    bool active;       /* is the condition currently true? */
} persist_t;

typedef struct
{
    persist_t p;
    float ref; /* value at the start of the window */
} stable_t;

// Returns true once cond has been continuously true for at least hold_ms
static bool persist_update(persist_t *p, bool cond, uint32_t now_millis, uint32_t hold_ms)
{
    if (!cond)
    {
        p->active = false;
        return false;
    }

    if (!p->active)
    {
        p->active = true;
        p->since_ms = now_millis;
    }

    return (now_millis - p->since_ms) >= hold_ms;
}

// Returns true once value has stayed within tol of the value at the start of the window for at least hold_ms.
// If value drifts farther than tol the window restarts from the current value.
static bool stable_update(stable_t *s, float value, bool valid, float tol, uint32_t now_millis, uint32_t hold_ms)
{
    if (!valid)
    {
        s->p.active = false;
        return false;
    }

    if (!s->p.active || fabsf(value - s->ref) > tol)
    {
        s->ref = value;
        s->p.active = false;
    }

    return persist_update(&s->p, true, now_millis, hold_ms);
}

#pragma endregion

#pragma region state_machine

/* Detectors state, reset on every state change */
typedef struct
{
    persist_t light;
    persist_t dark;
    bool light_armed; /* IDLE only: the ejection is checked only after the rocket has been closed */
    persist_t ascent;
    float ground_alt_m; /* lowest altitude seen in IDLE, reference for the ascent */
    bool has_ground_alt;
    stable_t altitude;
    persist_t accel_still;
} detectors_t;

static detectors_t det;

static void reset_detectors(void)
{
    det = (detectors_t){0};
}

// reason is only used by the debug print, e.g. "light held 1000ms"
static void change_state(flight_state_t old_state, flight_state_t new_state, uint32_t now_millis, const char *reason)
{
    DBG_INFO("STATE %s -> %s (%s)", debug_flight_state_name(old_state), debug_flight_state_name(new_state), reason);
    ds_write_flight_state(new_state, now_millis);
    reset_detectors();
}

static bool is_there_light(datastore_t *ds)
{
    return (ds->fotores.s.valid && ds->fotores.is_there_light) ||
           (ds->uv.s.valid && ds->uv.is_there_light);
}

// Dark only if at least one light sensor is valid and none of them sees light
static bool is_dark(datastore_t *ds)
{
    return (ds->fotores.s.valid || ds->uv.s.valid) && !is_there_light(ds);
}

// The CanSat is turned on with the rocket open, so light in IDLE is ignored until it has
// been dark long enough to say the rocket is closed. Once armed it stays armed.
static bool is_light_armed(datastore_t *ds, uint32_t now_millis)
{
    if (!det.light_armed && persist_update(&det.dark, is_dark(ds), now_millis, LIGHT_ARM_DARK_HOLD_MS))
    {
        det.light_armed = true;
        DBG_INFO("ejection check armed (dark for %lums)", (unsigned long)(now_millis - det.dark.since_ms));
    }

    return det.light_armed;
}

static bool is_ascending(datastore_t *ds, uint32_t now_millis)
{
    bool above_ground = false;

    if (ds->env.s.valid)
    {
        // Tracking the minimum keeps the reference on the ground even if the first samples are noisy
        if (!det.has_ground_alt || ds->env.altitude_m < det.ground_alt_m)
        {
            det.ground_alt_m = ds->env.altitude_m;
            det.has_ground_alt = true;
        }

        above_ground = (ds->env.altitude_m - det.ground_alt_m) > ASCENT_ALT_DELTA_M;
    }

    return persist_update(&det.ascent, above_ground, now_millis, ASCENT_HOLD_MS);
}

// Returns true (and changes state) if the ejection was detected
static bool check_ejection(datastore_t *ds, uint32_t now_millis)
{
    if (!persist_update(&det.light, is_there_light(ds), now_millis, EJECTION_LIGHT_HOLD_MS))
        return false;

#ifdef SERIAL_DEBUG_INFO
    char reason[32];
    snprintf(reason, sizeof(reason), "light held %lums", (unsigned long)(now_millis - det.light.since_ms));
#else
    const char *reason = NULL;
#endif
    change_state(ds->state, FLIGHT_STATE_DESCENT, now_millis, reason);
    return true;
}

static bool is_landed(datastore_t *ds, uint32_t now_millis)
{
    // Both detectors must be updated every run, don't short circuit them
    bool alt_stable = stable_update(&det.altitude, ds->env.altitude_m, ds->env.s.valid,
                                    LANDED_ALT_TOL_M, now_millis, LANDED_HOLD_MS);

    bool accel_is_1g = ds->imu.s.valid && fabsf(ds->imu.accel_mag - 1.0f) <= LANDED_ACCEL_TOL_G;
    bool accel_still = persist_update(&det.accel_still, accel_is_1g, now_millis, LANDED_HOLD_MS);

    return alt_stable && accel_still;
}

uint32_t mission_state_task_run(task_t *self, uint32_t now_millis)
{
    datastore_t ds;
    if (!ds_read_all(&ds))
    {
        // A write is in progress, try again at the next run
        self->last_state = TASK_STATE_BUSY;
        return self->period;
    }

    self->last_state = TASK_STATE_OK;

    switch (ds.state)
    {
    case FLIGHT_STATE_BOOT:
        // Sensors are initialized in setup(), nothing else to do for now
        change_state(ds.state, FLIGHT_STATE_SELF_TEST, now_millis, "boot done");
        break;

    case FLIGHT_STATE_SELF_TEST:
        // TODO: no self test implemented yet
        change_state(ds.state, FLIGHT_STATE_IDLE, now_millis, "no self test yet");
        break;

    case FLIGHT_STATE_IDLE:
        // The ejection is still checked here: if the ascent is missed (e.g. no barometer) IDLE goes straight to DESCENT
        if (is_light_armed(&ds, now_millis) && check_ejection(&ds, now_millis))
            break;
        if (is_ascending(&ds, now_millis))
            change_state(ds.state, FLIGHT_STATE_ASCENT, now_millis, "altitude above ground");
        break;

    case FLIGHT_STATE_ASCENT:
        check_ejection(&ds, now_millis);
        break;

    case FLIGHT_STATE_DESCENT:
        if (is_landed(&ds, now_millis))
            change_state(ds.state, FLIGHT_STATE_LANDED, now_millis, "altitude and accel stable");
        break;

    case FLIGHT_STATE_LANDED:
        // Final state
        break;

    default:
        self->fail_count += 1;
        self->last_state = TASK_STATE_ERR;
        break;
    }

    return self->period;
}

void mission_state_task_on_enable(task_t *self)
{
    (void)self;
    reset_detectors();
}

#pragma endregion
