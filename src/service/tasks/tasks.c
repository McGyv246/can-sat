#include <stdint.h>
#include <stdbool.h>

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

bool tasks_enabled_core0[TASK_CORE0_COUNT] = {false};
bool tasks_enabled_core0_idle[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = false,
    [TASK_CORE0_IMU] = false,
    [TASK_CORE0_ENV] = false,
    [TASK_CORE0_GAS] = false,
    [TASK_CORE0_UV_FAST] = true,
    [TASK_CORE0_UV_SLOW] = false,
    [TASK_CORE0_FOTORES_FAST] = true,
    [TASK_CORE0_FOTORES_SLOW] = false,
    [TASK_CORE0_BATTERY] = true,
};
bool tasks_enabled_core0_descent[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = true,
    [TASK_CORE0_IMU] = true,
    [TASK_CORE0_ENV] = true,
    [TASK_CORE0_GAS] = true,
    [TASK_CORE0_UV_FAST] = false,
    [TASK_CORE0_UV_SLOW] = true,
    [TASK_CORE0_FOTORES_FAST] = false,
    [TASK_CORE0_FOTORES_SLOW] = true,
    [TASK_CORE0_BATTERY] = true,
};
bool tasks_enabled_core0_landed[TASK_CORE0_COUNT] = {
    [TASK_CORE0_GPS] = true,
    [TASK_CORE0_IMU] = false,
    [TASK_CORE0_ENV] = true,
    [TASK_CORE0_GAS] = true,
    [TASK_CORE0_UV_FAST] = false,
    [TASK_CORE0_UV_SLOW] = true,
    [TASK_CORE0_FOTORES_FAST] = false,
    [TASK_CORE0_FOTORES_SLOW] = true,
    [TASK_CORE0_BATTERY] = true,
};

bool tasks_enabled_core1[TASK_CORE1_COUNT] = {false};
bool tasks_enabled_core1_idle[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = true,
    [TASK_CORE1_LORA] = false,
};
bool tasks_enabled_core1_descent[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = true,
    [TASK_CORE1_LORA] = true,
};

bool tasks_enabled_core1_landed[TASK_CORE1_COUNT] = {
    [TASK_CORE1_SD] = true,
    [TASK_CORE1_LORA] = true,
};
