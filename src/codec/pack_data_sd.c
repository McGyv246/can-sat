#include <stdint.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "hal_time.h"
#include "config.h"
#include "crc.h"

static void build_header(sd_header_t *out, data_type_t type, uint16_t len, uint32_t now_millis)
{
    out->sync = SYNC_WORD;
    out->type = type;
    out->len = len;
    out->t_ms = now_millis;
}

static void pack_stamp(stamp_packed_t *out, stamp_t *data)
{
    out->t_ms = data->t_ms;
    out->valid = data->valid;
}

void env_to_sd(env_sd_t *out, env_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.temp_c = data->temp_c;
    out->data.pressure_pa = data->pressure_pa;
    out->data.humidity_pct = data->humidity_pct;
    out->data.altitude_m = data->altitude_m;

    sd_header_t header;
    build_header(&header, DATA_TYPE_ENV, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void imu_to_sd(imu_sd_t *out, imu_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    memcpy(out->data.accel, data->accel, sizeof(out->data.accel));
    memcpy(out->data.gyro, data->gyro, sizeof(out->data.gyro));
    out->data.accel_mag = data->accel_mag;

    sd_header_t header;
    build_header(&header, DATA_TYPE_IMU, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void gps_to_sd(gps_sd_t *out, gps_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.lat = data->lat;
    out->data.lon = data->lon;
    out->data.alt_m = data->alt_m;
    out->data.speed_ms = data->speed_ms;
    out->data.course_deg = data->course_deg;
    out->data.hdop = data->hdop;
    out->data.sats = data->sats;
    out->data.fix_type = data->fix_type;
    out->data.utc_s = data->utc_s;

    sd_header_t header;
    build_header(&header, DATA_TYPE_GPS, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void uv_to_sd(uv_sd_t *out, uv_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.is_there_light = (uint8_t)data->is_there_light;
    out->data.uv_raw = data->uv_raw;
    out->data.uv_index = data->uv_index;

    sd_header_t header;
    build_header(&header, DATA_TYPE_UV, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void fotores_to_sd(fotores_sd_t *out, fotores_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;
    out->data.is_there_light = (uint8_t)data->is_there_light;

    sd_header_t header;
    build_header(&header, DATA_TYPE_FOTORES, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void gas_to_sd(gas_sd_t *out, gas_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.raw = data->raw;
    out->data.heater_on_s = data->heater_on_s;
    out->data.heater_enabled = (uint8_t)data->heater_enabled;

    sd_header_t header;
    build_header(&header, DATA_TYPE_GAS, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void battery_to_sd(battery_sd_t *out, battery_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.voltage_raw = data->voltage_raw;
    out->data.voltage = data->voltage;
    out->data.percentage = data->percentage;
    out->data.is_alert_active = data->is_alert_active;

    sd_header_t header;
    build_header(&header, DATA_TYPE_BATTERY, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void stats_to_sd(stats_sd_t *out, datastore_t *data)
{
    out->data.t_ms = data->t_ms;
    out->data.state = (uint8_t)data->state;
    out->data.alt_peak_env = data->alt_peak_env;
    out->data.alt_peak_gps = data->alt_peak_gps;
    out->data.records_dropped = data->records_dropped;

    sd_header_t header;
    build_header(&header, DATA_TYPE_STATS, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

// °C -> 0.01 °C, clamped to the int16 range. NaN becomes THERMAL_PIXEL_INVALID
static int16_t pack_thermal_pixel(float c)
{
    if (c != c) // NaN is the only number that is not equal to itself
        return THERMAL_PIXEL_INVALID;

    float v = roundf(c * 100.0f);
    if (v < -INT16_MAX)
        return -INT16_MAX; // not INT16_MIN, that's THERMAL_PIXEL_INVALID
    if (v > INT16_MAX)
        return INT16_MAX;
    return (int16_t)v;
}

void thermal_to_sd(thermal_sd_t *out, thermal_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    for (size_t i = 0; i < THERMAL_PIXELS; i++)
        out->data.pixels[i] = pack_thermal_pixel(data->pixels[i]);

    sd_header_t header;
    build_header(&header, DATA_TYPE_THERMAL, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}
