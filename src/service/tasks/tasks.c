#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "types.h"
#include "tasks.h"

/*
All the tasks the scheduler runs, with their initial settings.
Fields not listed start at 0 / NULL (due = 0, fail_count = 0, last_state = TASK_STATE_OK, no on_enable/on_disable).
Tasks with a fast and a slow variant (UV, FOTORES) start with the slow period:
change self->period at runtime when switching flight phase (fast: 20 ms, priority 0 for UV).
*/
task_t tasks_core0[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = {
        .name = "GPS",
        .run = gps_task_run,
        .on_enable = gps_task_on_enable,
        .on_disable = gps_task_on_disable,
        .period = 500,
        .priority = 0,
    },
    [TASK_CORE0_IMU] = {
        .name = "IMU",
        .run = imu_task_run,
        .on_enable = imu_task_on_enable,
        .on_disable = imu_task_on_disable,
        .period = 50,
        .priority = 1,
    },
    [TASK_CORE0_ENV] = {
        .name = "GYBMEP",
        .run = gybmep_task_run,
        .on_enable = gybmep_task_on_enable,
        .on_disable = gybmep_task_on_disable,
        .period = 500,
        .priority = 2,
    },
    [TASK_CORE0_GAS] = {
        .name = "GAS",
        .run = mq_7_task_run,
        .on_enable = mq_7_task_on_enable,
        .on_disable = mq_7_task_on_disable,
        .period = 500,
        .priority = 1,
    },
    [TASK_CORE0_UV_FAST] = {
        .name = "UV_FAST",
        .run = uv_task_run,
        .period = 20,
        .priority = 0,
    },
    [TASK_CORE0_UV_SLOW] = {
        .name = "UV_SLOW",
        .run = uv_task_run,
        .period = 1000,
        .priority = 5,
    },
    [TASK_CORE0_FOTORES_FAST] = {
        .name = "FOTORES_FAST",
        .run = fotores_task_run,
        .period = 20,
        .priority = 0,
    },
    [TASK_CORE0_FOTORES_SLOW] = {
        .name = "FOTORES_SLOW",
        .run = fotores_task_run,
        .period = 1000,
        .priority = 4,
    },
    [TASK_CORE0_BATTERY] = {
        .name = "FUEL_GAUGE",
        .run = fuel_gauge_task_run,
        .on_enable = fuel_gauge_task_on_enable,
        .on_disable = fuel_gauge_task_on_disable,
        .period = 20,
        .priority = 6,
    },
    [TASK_CORE0_MISSION_STATE] = {
        .name = "MISSION_STATE",
        .run = mission_state_task_run,
        .on_enable = mission_state_task_on_enable,
        .period = 50,
        .priority = 1,
    },
    [TASK_CORE0_THERMAL] = {
        // the camera measures 2 subpages per second (a full frame every second): polling faster just finds no new data
        .name = "THERMAL",
        .run = thermal_task_run,
        .on_enable = thermal_task_on_enable,
        .period = 1000,
        .priority = 3,
    },
    [TASK_CORE0_LED] = {
        // 50 ms is enough for the shortest flash of the patterns (100 ms)
        .name = "LED",
        .run = led_task_run,
        .on_enable = led_task_on_enable,
        .on_disable = led_task_on_disable,
        .period = 50,
        .priority = 7,
    },
};

task_t tasks_core1[TASK_CORE1_COUNT] = {

    [TASK_CORE1_SD] = {
        .name = "SD",
        .run = sd_task_run,
        .period = 300,
        .priority = 0,
    },
    [TASK_CORE1_LORA] = {
        .name = "LORA",
        .run = lora_task_run,
        .on_enable = lora_task_on_enable,
        .on_disable = lora_task_on_disable,
        .period = 1500,
        .priority = 1,
    },
};

/*
Live enabled arrays owned by the schedulers: everything starts disabled,
the right profile is applied as soon as each core reads the flight state.
*/
bool tasks_enabled_core0[TASK_CORE0_COUNT] = {false};
bool tasks_enabled_core1[TASK_CORE1_COUNT] = {false};

/*
Per flight state profiles.
TASK_CORE0_MISSION_STATE must be enabled in every profile, otherwise the state would never change again.
TASK_CORE0_LED is enabled in every profile too: it decides by itself what to show in each state.
*/
const bool tasks_enabled_core0_boot[TASK_CORE0_COUNT] = {
    [TASK_CORE0_MISSION_STATE] = true,
    [TASK_CORE0_LED] = true,
};
const bool tasks_enabled_core0_idle[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = false,
    [TASK_CORE0_IMU] = false,
    [TASK_CORE0_ENV] = false,
    [TASK_CORE0_GAS] = false,
    [TASK_CORE0_UV_FAST] = true,
    [TASK_CORE0_UV_SLOW] = false,
    [TASK_CORE0_FOTORES_FAST] = true,
    [TASK_CORE0_FOTORES_SLOW] = false,
    [TASK_CORE0_BATTERY] = true,
    [TASK_CORE0_MISSION_STATE] = true,
    [TASK_CORE0_THERMAL] = false,
    [TASK_CORE0_LED] = true,
};
/* Like IDLE, plus the thermal camera: it boots in ASCENT so it's already running at the ejection */
const bool tasks_enabled_core0_ascent[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = false,
    [TASK_CORE0_IMU] = false,
    [TASK_CORE0_ENV] = false,
    [TASK_CORE0_GAS] = false,
    [TASK_CORE0_UV_FAST] = true,
    [TASK_CORE0_UV_SLOW] = false,
    [TASK_CORE0_FOTORES_FAST] = true,
    [TASK_CORE0_FOTORES_SLOW] = false,
    [TASK_CORE0_BATTERY] = true,
    [TASK_CORE0_MISSION_STATE] = true,
    [TASK_CORE0_THERMAL] = true,
    [TASK_CORE0_LED] = true,
};
const bool tasks_enabled_core0_descent[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = true,
    [TASK_CORE0_IMU] = true,
    [TASK_CORE0_ENV] = true,
    [TASK_CORE0_GAS] = true,
    [TASK_CORE0_UV_FAST] = false,
    [TASK_CORE0_UV_SLOW] = true,
    [TASK_CORE0_FOTORES_FAST] = false,
    [TASK_CORE0_FOTORES_SLOW] = true,
    [TASK_CORE0_BATTERY] = true,
    [TASK_CORE0_MISSION_STATE] = true,
    [TASK_CORE0_THERMAL] = true,
    [TASK_CORE0_LED] = true,
};
const bool tasks_enabled_core0_landed[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = true,
    [TASK_CORE0_IMU] = false,
    [TASK_CORE0_ENV] = true,
    [TASK_CORE0_GAS] = true,
    [TASK_CORE0_UV_FAST] = false,
    [TASK_CORE0_UV_SLOW] = true,
    [TASK_CORE0_FOTORES_FAST] = false,
    [TASK_CORE0_FOTORES_SLOW] = true,
    [TASK_CORE0_BATTERY] = true,
    [TASK_CORE0_MISSION_STATE] = true,
    [TASK_CORE0_THERMAL] = false,
    [TASK_CORE0_LED] = true,
};

const bool tasks_enabled_core1_boot[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = false,
    [TASK_CORE1_LORA] = false,
};
const bool tasks_enabled_core1_idle[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = true,
    [TASK_CORE1_LORA] = false,
};
const bool tasks_enabled_core1_descent[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = true,
    [TASK_CORE1_LORA] = true,
};
const bool tasks_enabled_core1_landed[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = true,
    [TASK_CORE1_LORA] = true,
};

/* Which profile to apply in each flight state. On core 1 ASCENT is short and behaves like IDLE */
const bool *const tasks_profile_core0[FLIGHT_STATE_COUNT] = {
    [FLIGHT_STATE_BOOT] = tasks_enabled_core0_boot,
    [FLIGHT_STATE_SELF_TEST] = tasks_enabled_core0_boot,
    [FLIGHT_STATE_IDLE] = tasks_enabled_core0_idle,
    [FLIGHT_STATE_ASCENT] = tasks_enabled_core0_ascent,
    [FLIGHT_STATE_DESCENT] = tasks_enabled_core0_descent,
    [FLIGHT_STATE_LANDED] = tasks_enabled_core0_landed,
};
const bool *const tasks_profile_core1[FLIGHT_STATE_COUNT] = {
    [FLIGHT_STATE_BOOT] = tasks_enabled_core1_boot,
    [FLIGHT_STATE_SELF_TEST] = tasks_enabled_core1_boot,
    [FLIGHT_STATE_IDLE] = tasks_enabled_core1_idle,
    [FLIGHT_STATE_ASCENT] = tasks_enabled_core1_idle,
    [FLIGHT_STATE_DESCENT] = tasks_enabled_core1_descent,
    [FLIGHT_STATE_LANDED] = tasks_enabled_core1_landed,
};
