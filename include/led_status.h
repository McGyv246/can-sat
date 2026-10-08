#ifndef LED_STATUS_H
#define LED_STATUS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h"

    /*
    What the RGB led shows, from the highest priority to the lowest.
    Errors and warnings are shown only on the ground before the launch (BOOT, SELF_TEST, IDLE):
    in ASCENT and DESCENT the led stays off (nobody sees it, and in ASCENT its light could fake
    the ejection to the light sensors), in LANDED it only blinks to help find the can.
    */
    typedef enum
    {
        LED_STATUS_OFF,            /* ASCENT, DESCENT */
        LED_STATUS_LANDED,         /* white flash every 2 s, to find the can */
        LED_STATUS_ERROR,          /* fast red blink: a task died or the SD failed, don't launch */
        LED_STATUS_BATTERY_LOW,    /* yellow blink */
        LED_STATUS_SENSOR_WARNING, /* violet blink: the last run of an enabled sensor task failed */
        LED_STATUS_BOOT,           /* steady blue: BOOT, SELF_TEST */
        LED_STATUS_READY,          /* green flash every 2 s: IDLE, ready for the launch */
        LED_STATUS_COUNT,          /* MUST BE LAST! */
    } led_status_t;

    /* Everything led_status_choose looks at, gathered by the led task */
    typedef struct
    {
        flight_state_t state;
        bool error;          /* a task was disabled after too many fails, or the last SD write failed */
        bool battery_low;    /* see led_battery_low */
        bool sensor_warning; /* see led_sensor_warning */
    } led_inputs_t;

    /*
    Groups of `blinks` flashes (on_ms on, off_ms off) followed by pause_ms off, repeated.
    off_ms = 0 and pause_ms = 0 means steady on, blinks = 0 means always off.
    */
    typedef struct
    {
        rgb_colors_t color;
        uint16_t on_ms;
        uint16_t off_ms;
        uint8_t blinks;
        uint16_t pause_ms;
    } led_pattern_t;

    extern const led_pattern_t led_patterns[LED_STATUS_COUNT];

    led_status_t led_status_choose(const led_inputs_t *in);

    // Is the led on elapsed_ms after the pattern started?
    bool led_pattern_is_on(const led_pattern_t *p, uint32_t elapsed_ms);

    // true if any task has more than SCHED_MAX_FAILS fails: the scheduler disables it at its next failure
    bool led_any_task_dead(const task_t *tasks, size_t n);

    // true if an enabled task failed its last run
    bool led_sensor_warning(const task_t *tasks, const bool *enabled, size_t n);

    bool led_battery_low(const battery_t *battery);

#ifdef __cplusplus
}
#endif

#endif
