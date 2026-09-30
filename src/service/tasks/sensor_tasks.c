#include <stdint.h>

#include "config.h"
#include "types.h"
#include "tasks.h"
#include "datastore.h"
#include "ring_buffer.h"
#include "hal_time.h"

#include "fotores_drivers.h"
#include "fuel_gauge_drivers.h"
#include "gps_drivers.h"
#include "gybmep_drivers.h"
#include "imu_drivers.h"
#include "mq_7_drivers.h"
#include "uv_drivers.h"

#pragma region helpers

// Updates the task state from the result of a sensor poll and returns the reschedule interval
static uint32_t task_report(task_t *self, sens_state_t state)
{
    switch (state)
    {
    case SENS_STATE_READY:
        self->last_state = TASK_STATE_OK;
        break;
    case SENS_STATE_NOT_READY:
        // TODO: eventually return a different reschedule interval in case it's busy
        self->last_state = TASK_STATE_BUSY;
        break;
    default:
        self->fail_count += 1;
        self->last_state = TASK_STATE_ERR;
        break;
    }

    return self->period;
}

// Counts a failure if a wake/sleep call went wrong
static void task_check(task_t *self, sens_state_t state)
{
    if (state == SENS_STATE_ERR)
        self->fail_count += 1;
}

#pragma endregion

#pragma region fotores

uint32_t fotores_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = fotores_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        fotores_t data;
        fotores_get(&data);

        ds_write_fotores(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

#pragma endregion

#pragma region fuel_gauge

uint32_t fuel_gauge_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = fuel_gauge_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        battery_t data;
        fuel_gauge_get(&data);

        ds_write_battery(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

void fuel_gauge_task_on_enable(task_t *self) { task_check(self, fuel_gauge_wake()); }
void fuel_gauge_task_on_disable(task_t *self) { task_check(self, fuel_gauge_sleep()); }

#pragma endregion

#pragma region gps

uint32_t gps_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = gps_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        gps_t data;
        gps_get(&data);

        ds_write_gps(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

void gps_task_on_enable(task_t *self) { task_check(self, gps_wake()); }
void gps_task_on_disable(task_t *self) { task_check(self, gps_sleep()); }

#pragma endregion

#pragma region gybmep

uint32_t gybmep_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = gybmep_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        env_t data;
        gybmep_get(&data);

        ds_write_env(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

void gybmep_task_on_enable(task_t *self) { task_check(self, gybmep_wake()); }
void gybmep_task_on_disable(task_t *self) { task_check(self, gybmep_sleep()); }

#pragma endregion

#pragma region imu

uint32_t imu_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = imu_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        imu_t data;
        imu_get(&data);

        ds_write_imu(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

void imu_task_on_enable(task_t *self) { task_check(self, imu_wake()); }
void imu_task_on_disable(task_t *self) { task_check(self, imu_sleep()); }

#pragma endregion

#pragma region mq_7

uint32_t mq_7_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = mq_7_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        gas_t data;
        mq_7_get(&data);

        ds_write_gas(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

void mq_7_task_on_enable(task_t *self)
{
    mq_7_wake(hal_millis()); // It might be better to make this in some other way, but don't care
}

void mq_7_task_on_disable(task_t *self)
{
    mq_7_sleep();
}

#pragma endregion

#pragma region uv

uint32_t uv_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = uv_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        uv_t data;
        uv_get(&data);

        ds_write_uv(&data, now_millis);
        buf_write(&data, sizeof(data));
    }

    return task_report(self, state);
}

#pragma endregion
