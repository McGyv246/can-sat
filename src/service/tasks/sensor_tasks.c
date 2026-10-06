#include <stdint.h>

#include "config.h"
#include "types.h"
#include "tasks.h"
#include "datastore.h"
#include "ring_buffer.h"
#include "hal_time.h"
#include "pack_data_sd.h"
#include "thermal_summary.h"
#include "gps_warmup.h"
#include "debug_print.h"

#include "fotores_drivers.h"
#include "fuel_gauge_drivers.h"
#include "gps_drivers.h"
#include "gybmep_drivers.h"
#include "imu_drivers.h"
#include "mq_7_drivers.h"
#include "mlx_drivers.h"
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

        fotores_sd_t packed_data;
        fotores_to_sd(&packed_data, &data);
        buf_write(&packed_data, sizeof(packed_data));
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

        battery_sd_t packed_data;
        battery_to_sd(&packed_data, &data);
        buf_write(&packed_data, sizeof(packed_data));
    }

    return task_report(self, state);
}

void fuel_gauge_task_on_enable(task_t *self) { task_check(self, fuel_gauge_wake()); }
void fuel_gauge_task_on_disable(task_t *self) { task_check(self, fuel_gauge_sleep()); }

#pragma endregion

#pragma region gps

/*
The task doesn't keep the GPS always on: gps_warmup decides it from the flight state (see gps_warmup.h).
In IDLE it's turned on until it locks, then off with the satellites saved; in the other states it's on.
*/
static gps_warmup_t gps_warmup;
static bool gps_powered;
static bool gps_fix_valid; /* valid field of the last sample, kept between the runs without a new one */

static void gps_set_power(task_t *self, bool on)
{
    if (on == gps_powered)
        return;

    task_check(self, on ? gps_wake() : gps_sleep());
    gps_powered = on;
    gps_fix_valid = false;
    DBG_INFO("GPS powered %s", on ? "on" : "off");
}

uint32_t gps_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = SENS_STATE_NOT_READY;

    if (gps_powered)
    {
        state = gps_poll(now_millis);

        if (state == SENS_STATE_READY)
        {
            gps_t data;
            gps_get(&data);
            ds_write_gps(&data, now_millis);

            gps_sd_t packed_data;
            gps_to_sd(&packed_data, &data);
            buf_write(&packed_data, sizeof(packed_data));

            if (data.s.valid != gps_fix_valid)
                DBG_INFO("GPS fix %s (%u sats)", data.s.valid ? "found" : "lost", (unsigned)data.sats);
            gps_fix_valid = data.s.valid;
        }
    }

    bool was_warm = gps_warmup.warm;
    gps_set_power(self, gps_warmup_update(&gps_warmup, ds_read_state(), gps_fix_valid, now_millis));
    if (gps_warmup.warm && !was_warm)
        DBG_INFO("GPS locked, satellites saved");

    if (!gps_powered)
    {
        // Off on purpose, waiting in IDLE: not a failure
        self->last_state = TASK_STATE_OK;
        return self->period;
    }

    return task_report(self, state);
}

// The GPS is powered by the first run, gps_warmup decides if it must be
void gps_task_on_enable(task_t *self)
{
    (void)self;
    gps_warmup_reset(&gps_warmup);
}

void gps_task_on_disable(task_t *self) { gps_set_power(self, false); }

bool gps_task_is_warm(uint32_t now_millis) { return gps_warmup_is_warm(&gps_warmup, now_millis); }

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

        env_sd_t packed_data;
        env_to_sd(&packed_data, &data);
        buf_write(&packed_data, sizeof(packed_data));
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

        imu_sd_t packed_data;
        imu_to_sd(&packed_data, &data);
        buf_write(&packed_data, sizeof(packed_data));
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

        gas_sd_t packed_data;
        gas_to_sd(&packed_data, &data);
        buf_write(&packed_data, sizeof(packed_data));
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

        uv_sd_t packed_data;
        uv_to_sd(&packed_data, &data);
        buf_write(&packed_data, sizeof(packed_data));
    }

    return task_report(self, state);
}

#pragma endregion

#pragma region thermal

uint32_t thermal_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = mlx_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        // static: the frame is 3 KB and its SD record 1.5 KB, too much for the stack
        static thermal_t frame;
        static thermal_sd_t packed_data;

        mlx_get(&frame);

        thermal_summary_t summary;
        thermal_summarize(&summary, &frame);
        ds_write_thermal(&summary, now_millis);

        thermal_to_sd(&packed_data, &frame);
        buf_write(&packed_data, sizeof(packed_data));
    }

    return task_report(self, state);
}

// The camera has no sleep mode: enabling the task boots it, disabling it just stops the reads
void thermal_task_on_enable(task_t *self) { task_check(self, mlx_init()); }

#pragma endregion
