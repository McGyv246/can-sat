#include <stdint.h>
#include <stdbool.h>

#include "types.h"

// This should initialize everything at 0 but needs to be tested
static datastore_t ds = {.state = BOOT};
static volatile uint8_t ds_seq = 0;

static void ds_write_begin();
static void ds_write_end(uint32_t now_millis);

void ds_write_env(env_t *data, uint32_t now_millis);
void ds_write_imu(imu_t *data, uint32_t now_millis);
void ds_write_gps(gps_t *data, uint32_t now_millis);
void ds_write_light(light_t *data, uint32_t now_millis);
void ds_write_gas(gas_t *data, uint32_t now_millis);
void ds_write_flight_state(flight_state_t data, uint32_t now_millis);

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

    ds.env.temp_c = data->temp_c;
    ds.env.pressure_pa = data->pressure_pa;
    ds.env.humidity_pct = data->humidity_pct;
    ds.env.altitude_m = data->altitude_m;

    if (data->altitude_m > ds.alt_peak_env)
    {
        ds.alt_peak_env = data->altitude_m;
    }

    // TODO: eventually remove these if handled by sensors
    ds.env.s.t_ms = now_millis;
    ds.env.s.valid = true;

    ds_write_end(now_millis);
}

void ds_write_imu(imu_t *data, uint32_t now_millis)
{
    ds_write_begin();

    for (size_t i = 0; i < 3; i++)
    {
        ds.imu.accel[i] = data->accel[i];
        ds.imu.gyro[i] = data->gyro[i];
        ds.imu.mag[i] = data->mag[i];
    }

    for (size_t i = 0; i < 4; i++)
    {
        ds.imu.quat[i] = data->quat[i];
    }

    ds.imu.accel_mag = data->accel_mag;

    // TODO: eventually remove these if handled by sensors
    ds.imu.s.t_ms = now_millis;
    ds.imu.s.valid = true;

    ds_write_end(now_millis);
}

void ds_write_gps(gps_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.gps.lat = data->lat;
    ds.gps.lon = data->lon;

    ds.gps.alt_m = data->alt_m;
    if (data->alt_m > ds.alt_peak_gps)
    {
        ds.alt_peak_gps = data->alt_m;
    }

    ds.gps.speed_ms = data->speed_ms;
    ds.gps.course_deg = data->course_deg;
    ds.gps.hdop = data->hdop;

    ds.gps.sats = data->sats;
    ds.gps.fix_type = data->fix_type;
    ds.gps.utc_s = data->utc_s;

    // TODO: eventually remove these if handled by sensors
    ds.gps.s.t_ms = now_millis;
    ds.gps.s.valid = true;

    ds_write_end(now_millis);
}

void ds_write_light(light_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.light.is_there_light = data->is_there_light;
    ds.light.uv_raw = data->uv_raw;
    ds.light.uv_index = data->uv_index;

    // TODO: eventually remove these if handled by sensors
    ds.light.s.t_ms = now_millis;
    ds.light.s.valid = true;

    ds_write_end(now_millis);
}

void ds_write_gas(gas_t *data, uint32_t now_millis)
{
    ds_write_begin();

    ds.gas.raw = data->raw;
    ds.gas.heater_on_s = data->heater_on_s;
    ds.gas.heater_enabled = data->heater_enabled;

    // TODO: eventually remove these if handled by sensors
    ds.gas.s.t_ms = now_millis;
    ds.gas.s.valid = true;

    ds_write_end(now_millis);
}

void ds_write_flight_state(flight_state_t data, uint32_t now_millis)
{
    ds_write_begin();

    ds.state = data;

    ds_write_end(now_millis);
}
