#include <stdint.h>
#include <stdio.h>
#include <math.h>

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

#ifdef SERIAL_DEBUG_INFO
#define GPS_DEBUG_PRINT_MS 5000

/*
Every GPS_DEBUG_PRINT_MS while the GPS is on, three lines to see why it doesn't lock (see the README), e.g.
  GPS on 125s | rx 452B/s bad 0 ovf 0 boots 1 ant OK | mode 1 view 9 used 0 hdop 99.9 time no
  GPS snr (PRN:dB-Hz/elev) orbit 2/9 05:31/42 12:0/-- 15:24/17 ...
  GPS hw | jam 4/255 state ok | noise 87 agc 2950/8191 | monitor on | replies 24/25
*/
static void gps_debug_print(uint32_t now_millis)
{
    static uint32_t last_print_ms;
    static uint32_t last_chars;

    if ((now_millis - last_print_ms) < GPS_DEBUG_PRINT_MS)
        return;
    uint32_t elapsed_ms = now_millis - last_print_ms;
    last_print_ms = now_millis;

    gps_debug_t d;
    gps_get_debug(&d);
    gps_t data;
    gps_get(&data);

    uint32_t rate = (d.chars - last_chars) * 1000u / elapsed_ms;
    last_chars = d.chars;
    unsigned hdop_x10 = isnan(data.hdop) ? 999u : (unsigned)(data.hdop * 10.0f + 0.5f);

    DBG_INFO("GPS on %lus | rx %luB/s bad %lu ovf %lu boots %lu ant %s | mode %u view %u used %u hdop %u.%u time %s",
             (unsigned long)(d.on_ms / 1000u), (unsigned long)rate, (unsigned long)d.failed_checksum,
             (unsigned long)d.rx_overflows, (unsigned long)d.boots, d.antenna[0] ? d.antenna : "?",
             (unsigned)d.fix_mode, (unsigned)d.sats_in_view, (unsigned)data.sats, hdop_x10 / 10u, hdop_x10 % 10u,
             d.time_valid ? "ok" : "no");

    // " PPP:SS/EE" per satellite, elevation "--" if the module has no orbit for it yet
    char sats[GPS_DEBUG_MAX_SATS * 11 + 1];
    size_t len = 0;
    unsigned with_orbit = 0;
    sats[0] = '\0';
    for (uint8_t i = 0; i < d.sats_count && len < sizeof(sats); i++)
    {
        if (d.elev[i] >= 0)
        {
            with_orbit++;
            len += (size_t)snprintf(sats + len, sizeof(sats) - len, " %02u:%u/%d", (unsigned)d.prn[i],
                                    (unsigned)d.snr[i], (int)d.elev[i]);
        }
        else
            len += (size_t)snprintf(sats + len, sizeof(sats) - len, " %02u:%u/--", (unsigned)d.prn[i],
                                    (unsigned)d.snr[i]);
    }
    if (d.sats_count)
        DBG_INFO("GPS snr (PRN:dB-Hz/elev) orbit %u/%u%s", with_orbit, (unsigned)d.sats_count, sats);
    else
        DBG_INFO("GPS snr (PRN:dB-Hz/elev) none in view");

    // Interference measured by the module, from the reply to the request of the previous print
    static const char *const jam_states[] = {"?", "ok", "WARNING", "CRITICAL"};
    static const char *const itfm_states[] = {"not sent", "no answer", "on", "NAK"};
    if (d.hw_replies)
        DBG_INFO("GPS hw | jam %u/255 state %s | noise %u agc %u/8191 | monitor %s | replies %lu/%lu",
                 (unsigned)d.jam_ind, jam_states[d.jam_state & 3], (unsigned)d.noise_per_ms, (unsigned)d.agc_cnt,
                 itfm_states[d.itfm_ack & 3], (unsigned long)d.hw_replies, (unsigned long)d.hw_requests);
    else
        DBG_INFO("GPS hw | no MON-HW reply yet (%lu requests): if it stays 0 the GPS RX wire (board TX) may be missing",
                 (unsigned long)d.hw_requests);
    gps_request_hw_status();
}
#endif

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

#ifdef SERIAL_DEBUG_INFO
            if (data.s.valid != gps_fix_valid)
            {
                gps_debug_t d;
                gps_get_debug(&d);
                DBG_INFO("GPS fix %s (%u sats, %lus after power on)", data.s.valid ? "found" : "lost",
                         (unsigned)data.sats, (unsigned long)(d.on_ms / 1000u));
            }
#endif
            gps_fix_valid = data.s.valid;
        }

#ifdef SERIAL_DEBUG_INFO
        gps_debug_print(now_millis);
#endif
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

    // The task doesn't run anymore to write the datastore: publish the heater off here,
    // otherwise heater_enabled / heater_on_s stay at the last sample taken with the heater on
    gas_t data;
    mq_7_get(&data);
    ds_write_gas(&data, hal_millis());
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
