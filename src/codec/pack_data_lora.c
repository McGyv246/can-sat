/*
!!THIS IS JUST A DRAFT!!
It's AI generated and I don't fully understand what's going on.
*/

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "datastore.h"
#include "config.h"
#include "crc.h"

#define LORA_VERSION 1 /* increase it whenever the packet layout changes */

/* Bits of the "status" field: 1 = that block's last read succeeded. */
#define STATUS_ENV_OK (1u << 0)
#define STATUS_IMU_OK (1u << 1)
#define STATUS_GPS_OK (1u << 2)
#define STATUS_UV_OK (1u << 3)
#define STATUS_FOTORES_OK (1u << 4)
#define STATUS_GAS_OK (1u << 5)
#define STATUS_BATTERY_OK (1u << 6)
#define STATUS_BATTERY_ALERT (1u << 7)
#define STATUS_HEATER_ON (1u << 8)

/* raw = round((x - offset) * scale), then forced into the range [lo, hi]. */
static int32_t quantize_clamped(double x, double scale, double offset, int32_t lo, int32_t hi)
{
    double v = (x - offset) * scale;

    if (v != v) /* NaN is the only number that is not equal to itself */
        return lo;

    v = floor(v + 0.5); /* round to nearest */

    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return (int32_t)v;
}

void ds_to_lora(data_to_lora_t *out, datastore_t *data)
{
    static uint16_t seq = 0; /* +1 every packet, wraps around after 65535 */

    memset(out, 0, sizeof(*out)); /* start with everything = 0 */

    /* ---------- header ---------- */
    out->sync = SYNC_WORD;
    out->version = LORA_VERSION;
    out->seq = seq++;

    /* ---------- flight info ---------- */
    out->ms_since_boot = data->t_ms;              /* as is, milliseconds */
    out->flight_state = (uint8_t)data->state;     /* as is, 0..5 */
    out->records_dropped = data->records_dropped; /* as is */

    /* ---------- environment ---------- */
    out->temp_c = (int16_t)quantize_clamped(data->env.temp_c, 100.0, 0.0, -32768, 32767);            /* 0.01 C */
    out->pressure = (uint16_t)quantize_clamped(data->env.pressure_pa, 0.5, 30000.0, 0, 65535);       /* Pa = raw * 2 + 30000 */
    out->humidity_pct = (uint8_t)quantize_clamped(data->env.humidity_pct, 2.0, 0.0, 0, 255);         /* 0.5 % */
    out->altitude_baro_m = (int16_t)quantize_clamped(data->env.altitude_m, 1.0, 0.0, -32768, 32767); /* 1 m */

    /* ---------- IMU ---------- */
    /* No orientation is computed on the rocket. We send the raw sensor readings
     * and the ground station works out the orientation (see decode_lora.py). */
    for (int i = 0; i < 3; i++)
    {
        out->accel[i] = (int16_t)quantize_clamped(data->imu.accel[i], 1000.0, 0.0, -32768, 32767); /* 0.001 g */
        out->gyro[i] = (int16_t)quantize_clamped(data->imu.gyro[i], 10.0, 0.0, -32768, 32767);     /* 0.1 deg/s */
    }
    out->accel_mag = (int16_t)quantize_clamped(data->imu.accel_mag, 1000.0, 0.0, -32768, 32767); /* 0.001 g */

    /* ---------- light sensors ---------- */
    out->is_there_light_uv = data->uv.is_there_light ? 1 : 0;
    out->is_there_light_fotores = data->fotores.is_there_light ? 1 : 0;
    out->uv = (uint8_t)quantize_clamped(data->uv.uv_index, 10.0, 0.0, 0, 255); /* 0.1 UV index */

    /* ---------- gas sensor ---------- */
    out->gas_raw = data->gas.raw; /* as is */

    /* seconds since the heater was switched on (0 if off), max 255.
     * gas.heater_on_s is already the time the heater has been on, set by the driver. */
    {
        uint32_t elapsed = 0;

        if (data->gas.heater_enabled)
            elapsed = data->gas.heater_on_s;
        if (elapsed > 255u)
            elapsed = 255u;
        out->heater_on_since = (uint8_t)elapsed;
    }

    /* ---------- GPS ---------- */
    out->lat = quantize_clamped(data->gps.lat, 1e7, 0.0, INT32_MIN, INT32_MAX); /* degrees * 10 000 000 */
    out->lon = quantize_clamped(data->gps.lon, 1e7, 0.0, INT32_MIN, INT32_MAX);
    out->altitude_gps_m = (int16_t)quantize_clamped(data->gps.alt_m, 1.0, 0.0, -32768, 32767); /* 1 m */

    {
        /* satfix = satellites in the upper 5 bits, fix type in the lower 3 bits */
        uint8_t sats = data->gps.sats;
        uint8_t fix = data->gps.fix_type;

        if (sats > 31)
            sats = 31;
        if (fix > 7)
            fix = 7;
        out->satfix = (uint8_t)((sats << 3) | fix);
    }

    /* ---------- battery ---------- */
    out->vbatt = (uint8_t)quantize_clamped(data->battery.voltage, 100.0, 2.0, 0, 255); /* V = raw / 100 + 2.00 */

    /* ---------- status bits ---------- */
    {
        uint16_t status = 0;

        if (data->env.s.valid)
            status |= STATUS_ENV_OK;
        if (data->imu.s.valid)
            status |= STATUS_IMU_OK;
        if (data->gps.s.valid)
            status |= STATUS_GPS_OK;
        if (data->uv.s.valid)
            status |= STATUS_UV_OK;
        if (data->fotores.s.valid)
            status |= STATUS_FOTORES_OK;
        if (data->gas.s.valid)
            status |= STATUS_GAS_OK;
        if (data->battery.s.valid)
            status |= STATUS_BATTERY_OK;
        if (data->battery.is_alert_active)
            status |= STATUS_BATTERY_ALERT;
        if (data->gas.heater_enabled)
            status |= STATUS_HEATER_ON;
        out->status = status;
    }

    /* ---------- CRC: MUST be last, covers every byte before it ---------- */
    out->crc16 = crc16((const uint8_t *)out, sizeof(*out) - sizeof(out->crc16));
}