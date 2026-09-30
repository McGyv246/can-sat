#ifndef TASKS_H
#define TASKS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

    /*
    Index of each task inside the tasks array.
    Use these with the scheduler functions, e.g. sched_change_enabled_i(&sched, false, TASK_CORE0_GPS)
    */
    typedef enum
    {
        TASK_CORE0_GPS,
        TASK_CORE0_IMU,
        TASK_CORE0_ENV,
        TASK_CORE0_GAS,
        TASK_CORE0_UV_FAST,
        TASK_CORE0_UV_SLOW,
        TASK_CORE0_FOTORES_FAST,
        TASK_CORE0_FOTORES_SLOW,
        TASK_CORE0_BATTERY,
        TASK_CORE0_COUNT, /* MUST BE LAST! */
    } task_core0_id_t;

    typedef enum
    {
        TASK_CORE1_SD,
        TASK_CORE1_LORA,
        TASK_CORE1_COUNT,
    } task_core1_id_t;

    /* Defined in tasks.c, to be passed to sched_init */
    extern task_t tasks_core0[TASK_CORE0_COUNT];
    extern bool tasks_enabled_core0[TASK_CORE0_COUNT];

    extern task_t tasks_core1[TASK_CORE1_COUNT];
    extern bool tasks_enabled_core1[TASK_CORE1_COUNT];

#pragma region task_callbacks
    /* Only needed by the tasks table in tasks.c */

    uint32_t fotores_task_run(task_t *self, uint32_t now_millis);

    uint32_t fuel_gauge_task_run(task_t *self, uint32_t now_millis);
    void fuel_gauge_task_on_enable(task_t *self);
    void fuel_gauge_task_on_disable(task_t *self);

    uint32_t gps_task_run(task_t *self, uint32_t now_millis);
    void gps_task_on_enable(task_t *self);
    void gps_task_on_disable(task_t *self);

    uint32_t gybmep_task_run(task_t *self, uint32_t now_millis);
    void gybmep_task_on_enable(task_t *self);
    void gybmep_task_on_disable(task_t *self);

    uint32_t imu_task_run(task_t *self, uint32_t now_millis);
    void imu_task_on_enable(task_t *self);
    void imu_task_on_disable(task_t *self);

    uint32_t mq_7_task_run(task_t *self, uint32_t now_millis);
    void mq_7_task_on_enable(task_t *self);
    void mq_7_task_on_disable(task_t *self);

    uint32_t uv_task_run(task_t *self, uint32_t now_millis);

    uint32_t lora_task_run(task_t *self, uint32_t now_millis);
    void lora_task_on_enable(task_t *self);
    void lora_task_on_disable(task_t *self);

    uint32_t sd_task_run(task_t *self, uint32_t now_millis);

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif
