#include <stdint.h>
#include <stdbool.h>

#include "types.h"

// This should initialize everything at 0 but needs to be tested
static datastore_t ds = {.state = FLIGHT_STATE_BOOT};
static volatile uint8_t ds_seq = 0;

static void ds_write_begin();
static void ds_write_end(uint32_t now_millis);

// ONLY for testing purposes
void _ds_set_full(datastore_t *new_ds);

void ds_write_env(env_t *data, uint32_t now_millis);
void ds_write_imu(imu_t *data, uint32_t now_millis);
void ds_write_gps(gps_t *data, uint32_t now_millis);
void ds_write_uv(uv_t *data, uint32_t now_millis);
void ds_write_gas(gas_t *data, uint32_t now_millis);
void ds_write_flight_state(flight_state_t data, uint32_t now_millis);

bool ds_read_all(datastore_t *data);

// TODO: remove this, it's just a placeholder for when the
// ring buffer will have the correct saving functions
// Maybe it shouldn't even stay here but it should be called
// by the code that calls writes in the datastore
static void temp_write_ring_buffer() {};

#pragma region write_fn

void _ds_set_full(datastore_t *new_ds)
{
    ds = *new_ds;
}

static void ds_write_begin()
{
    ds_seq++;
    __sync_synchronize(); // Not sure it works
}

static void ds_write_end(uint32_t now_millis)
{
    ds.t_ms = now_millis;
    __sync_synchronize(); // Not sure it works
    ds_seq++;
}

void ds_write_env(env_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.env = *data;

    if (data->altitude_m > ds.alt_peak_env)
    {
        ds.alt_peak_env = data->altitude_m;
    }

    // TODO: eventually remove these if handled by sensors
    // ds.env.s.t_ms = now_millis;
    // ds.env.s.valid = true;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

void ds_write_imu(imu_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.imu = *data;

    // TODO: eventually remove these if handled by sensors
    // ds.imu.s.t_ms = now_millis;
    // ds.imu.s.valid = true;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

void ds_write_gps(gps_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.gps = *data;

    if (data->alt_m > ds.alt_peak_gps)
    {
        ds.alt_peak_gps = data->alt_m;
    }

    // TODO: eventually remove these if handled by sensors
    // ds.gps.s.t_ms = now_millis;
    // ds.gps.s.valid = true;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

void ds_write_uv(uv_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.uv = *data;

    // TODO: eventually remove these if handled by sensors
    // ds.uv.s.t_ms = now_millis;
    // ds.uv.s.valid = true;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

void ds_write_gas(gas_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.gas = *data;

    // TODO: eventually remove these if handled by sensors
    // ds.gas.s.t_ms = now_millis;
    // ds.gas.s.valid = true;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

void ds_write_flight_state(flight_state_t data, uint32_t now_millis)
{
    ds_write_begin();

    ds.state = data;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

void ds_write_increment_records_dropped(uint32_t now_millis)
{
    ds_write_begin();

    ds.records_dropped += 1;

    temp_write_ring_buffer();

    ds_write_end(now_millis);
}

#pragma endregion

#pragma region read_fn

bool ds_read_all(datastore_t *data)
{
    if (ds_seq % 2 != 0)
    {
        return false;
    }

    *data = ds;

    if (ds_seq % 2 != 0)
    {
        return false;
    }

    return true;
}

#pragma endregion
