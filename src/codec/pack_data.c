#include <stdint.h>
#include <string.h>

#include "types.h"
#include "hal_time.h"
#include "config.h"
#include "crc.h"

static void build_header(sd_header_t *out, sensor_type_t type, uint16_t len, uint32_t now_millis)
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
    build_header(&header, ENV, (uint16_t)sizeof(out->data), hal_millis());

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
    build_header(&header, IMU_TYPE, (uint16_t)sizeof(out->data), hal_millis());

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
    build_header(&header, GPS, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}

void light_to_sd(light_sd_t *out, light_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.is_there_light = (uint8_t)data->is_there_light;
    out->data.uv_raw = data->uv_raw;
    out->data.uv_index = data->uv_index;

    sd_header_t header;
    build_header(&header, LIGHT, (uint16_t)sizeof(out->data), hal_millis());

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
    build_header(&header, GAS, (uint16_t)sizeof(out->data), hal_millis());

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
    build_header(&header, STATS, (uint16_t)sizeof(out->data), hal_millis());

    out->header = header;

    // exclude the crc itself from the crc computation
    out->crc = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc));
}