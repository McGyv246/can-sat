#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "types.h"
#include "led_status.h"

/*
Every on_ms shown in IDLE must stay well below EJECTION_LIGHT_HOLD_MS: if the led light reaches the
light sensors it must never look like the continuous light of the ejection (see led_status_test.c).
The only steady pattern (BOOT) is shown only while the light sensor tasks are disabled.
*/
const led_pattern_t led_patterns[LED_STATUS_COUNT] = {
    [LED_STATUS_OFF] = {0},
    [LED_STATUS_LANDED] = {.color = WHITE, .on_ms = 100, .blinks = 1, .pause_ms = 1900},
    [LED_STATUS_ERROR] = {.color = RED, .on_ms = 100, .off_ms = 100, .blinks = 1},
    [LED_STATUS_BATTERY_LOW] = {.color = YELLOW, .on_ms = 300, .off_ms = 300, .blinks = 1},
    [LED_STATUS_SENSOR_WARNING] = {.color = VIOLET, .on_ms = 300, .off_ms = 300, .blinks = 1},
    [LED_STATUS_BOOT] = {.color = BLUE, .on_ms = 1000, .blinks = 1},
    [LED_STATUS_GPS_SEARCHING] = {.color = CYAN, .on_ms = 100, .blinks = 1, .pause_ms = 900},
    [LED_STATUS_GPS_LOCKED] = {.color = GREEN, .on_ms = 100, .off_ms = 150, .blinks = 2, .pause_ms = 1500},
    [LED_STATUS_READY] = {.color = GREEN, .on_ms = 100, .blinks = 1, .pause_ms = 1900},
};

led_status_t led_status_choose(const led_inputs_t *in)
{
    switch (in->state)
    {
    case FLIGHT_STATE_ASCENT:
    case FLIGHT_STATE_DESCENT:
        return LED_STATUS_OFF;

    case FLIGHT_STATE_LANDED:
        return LED_STATUS_LANDED;

    case FLIGHT_STATE_BOOT:
    case FLIGHT_STATE_SELF_TEST:
    case FLIGHT_STATE_IDLE:
        if (in->error)
            return LED_STATUS_ERROR;
        if (in->battery_low)
            return LED_STATUS_BATTERY_LOW;
        if (in->sensor_warning)
            return LED_STATUS_SENSOR_WARNING;
        if (in->state != FLIGHT_STATE_IDLE)
            return LED_STATUS_BOOT;
        if (in->gps == LED_GPS_LOCKED)
            return LED_STATUS_GPS_LOCKED;
        if (in->gps == LED_GPS_SEARCHING)
            return LED_STATUS_GPS_SEARCHING;
        return LED_STATUS_READY;

    default:
        // not a valid flight state
        return LED_STATUS_ERROR;
    }
}

bool led_pattern_is_on(const led_pattern_t *p, uint32_t elapsed_ms)
{
    if (p->blinks == 0 || p->on_ms == 0)
        return false;

    uint32_t blink_ms = (uint32_t)p->on_ms + p->off_ms;
    uint32_t group_ms = blink_ms * p->blinks;
    uint32_t t = elapsed_ms % (group_ms + p->pause_ms);

    if (t >= group_ms)
        return false; // in the pause after the group

    return (t % blink_ms) < p->on_ms;
}

bool led_any_task_dead(const task_t *tasks, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (tasks[i].fail_count > SCHED_MAX_FAILS)
            return true;
    }
    return false;
}

bool led_sensor_warning(const task_t *tasks, const bool *enabled, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (enabled[i] && tasks[i].last_state == TASK_STATE_ERR)
            return true;
    }
    return false;
}

bool led_battery_low(const battery_t *battery)
{
    // the gauge clears its alert right after reading it, so the percentage is what keeps the led on
    return battery->s.valid && (battery->is_alert_active || battery->percentage < LED_BATTERY_LOW_PCT);
}

led_gps_t led_gps_status(const gps_t *gps, bool gps_enabled, uint32_t now_millis)
{
    if (!gps_enabled)
        return LED_GPS_OFF;

    // a sample every second is expected: an old valid one means the GPS stopped talking
    if (gps->s.valid && (now_millis - gps->s.t_ms) <= LED_GPS_STALE_MS)
        return LED_GPS_LOCKED;

    return LED_GPS_SEARCHING;
}
