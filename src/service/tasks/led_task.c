#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "types.h"
#include "tasks.h"
#include "datastore.h"
#include "led_status.h"
#include "rgb_drivers.h"

/*
Shows the satellite status on the RGB led (see led_status.h for the meaning of each pattern).
It runs on core 0, so tasks_core0 / tasks_enabled_core0 are read by the core that owns them.
tasks_core1 is owned by core 1: fail_count and last_state are single aligned words, so the worst
that can happen is reading a value one run old, which is fine for a led.
*/

static led_status_t current_status;
static uint32_t status_since_ms; /* when current_status started, so every pattern starts from its beginning */
static bool is_lit;

static void led_write(bool on, rgb_colors_t color)
{
    rgb_set_color(on ? color : BLACK);
    is_lit = on;
}

uint32_t led_task_run(task_t *self, uint32_t now_millis)
{
    datastore_t ds;
    if (!ds_read_all(&ds))
    {
        // A write is in progress, try again at the next run
        self->last_state = TASK_STATE_BUSY;
        return self->period;
    }

    led_inputs_t in = {
        .state = ds.state,
        .error = led_any_task_dead(tasks_core0, TASK_CORE0_COUNT) ||
                 led_any_task_dead(tasks_core1, TASK_CORE1_COUNT) ||
                 (tasks_enabled_core1[TASK_CORE1_SD] && tasks_core1[TASK_CORE1_SD].last_state == TASK_STATE_ERR),
        .battery_low = led_battery_low(&ds.battery),
        .sensor_warning = led_sensor_warning(tasks_core0, tasks_enabled_core0, TASK_CORE0_COUNT),
        .gps = led_gps_status(&ds.gps, tasks_enabled_core0[TASK_CORE0_GPS], now_millis),
    };

    led_status_t status = led_status_choose(&in);
    const led_pattern_t *pattern = &led_patterns[status];

    if (status != current_status)
    {
        current_status = status;
        status_since_ms = now_millis;
        led_write(led_pattern_is_on(pattern, 0), pattern->color);
    }
    else
    {
        // write only on changes, not every run
        bool on = led_pattern_is_on(pattern, now_millis - status_since_ms);
        if (on != is_lit)
            led_write(on, pattern->color);
    }

    self->last_state = TASK_STATE_OK;
    return self->period;
}

void led_task_on_enable(task_t *self)
{
    (void)self;
    rgb_init();
    is_lit = false;
    current_status = LED_STATUS_COUNT; // forces a write at the first run
}

void led_task_on_disable(task_t *self)
{
    (void)self;
    led_write(false, BLACK);
}
