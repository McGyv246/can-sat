/*
!!THIS IS JUST A DRAFT!!
It's AI generated and I don't fully understand what's going on.
*/

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pack_data_lora.h"
#include "config.h"
#include "crc.h"

/*
Conversion of every quantized field: raw = round((x - OFFSET) * SCALE), x = raw / SCALE + OFFSET.
Used by both ds_to_lora and lora_decode, so the two can't get out of sync.
*/
#define TEMP_SCALE 100.0 /* 0.01 C */
#define PRESSURE_SCALE 0.5 /* 2 Pa */
#define PRESSURE_OFFSET 30000.0
#define HUMIDITY_SCALE 2.0 /* 0.5 % */
#define ALT_BARO_SCALE 1.0 /* 1 m */
#define ACCEL_SCALE 1000.0 /* 0.001 g */
#define GYRO_SCALE 10.0    /* 0.1 deg/s */
#define UV_SCALE 10.0      /* 0.1 UV index */
#define LATLON_SCALE 1e7   /* 1e-7 degrees */
#define ALT_GPS_SCALE 1.0  /* 1 m */
#define VBATT_SCALE 100.0  /* 0.01 V */
#define VBATT_OFFSET 2.0

/* satfix = satellites in the upper 5 bits, fix type in the lower 3 bits */
#define SATFIX_FIX_BITS 3
#define SATFIX_FIX_MASK 0x07u
#define SATFIX_SATS_MAX 31u

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

/* Inverse of quantize_clamped (without the clamping, which can't be undone) */
static double dequantize(int32_t raw, double scale, double offset)
{
    return raw / scale + offset;
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
    out->temp_c = (int16_t)quantize_clamped(data->env.temp_c, TEMP_SCALE, 0.0, INT16_MIN, INT16_MAX);
    out->pressure = (uint16_t)quantize_clamped(data->env.pressure_pa, PRESSURE_SCALE, PRESSURE_OFFSET, 0, UINT16_MAX);
    out->humidity_pct = (uint8_t)quantize_clamped(data->env.humidity_pct, HUMIDITY_SCALE, 0.0, 0, UINT8_MAX);
    out->altitude_baro_m = (int16_t)quantize_clamped(data->env.altitude_m, ALT_BARO_SCALE, 0.0, INT16_MIN, INT16_MAX);

    /* ---------- IMU ---------- */
    /* No orientation is computed on the rocket. We send the raw sensor readings
     * and the ground station works out the orientation (see decode_lora.py). */
    for (int i = 0; i < 3; i++)
    {
        out->accel[i] = (int16_t)quantize_clamped(data->imu.accel[i], ACCEL_SCALE, 0.0, INT16_MIN, INT16_MAX);
        out->gyro[i] = (int16_t)quantize_clamped(data->imu.gyro[i], GYRO_SCALE, 0.0, INT16_MIN, INT16_MAX);
    }
    out->accel_mag = (int16_t)quantize_clamped(data->imu.accel_mag, ACCEL_SCALE, 0.0, INT16_MIN, INT16_MAX);

    /* ---------- light sensors ---------- */
    out->is_there_light_uv = data->uv.is_there_light ? 1 : 0;
    out->is_there_light_fotores = data->fotores.is_there_light ? 1 : 0;
    out->uv = (uint8_t)quantize_clamped(data->uv.uv_index, UV_SCALE, 0.0, 0, UINT8_MAX);

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
    out->lat = quantize_clamped(data->gps.lat, LATLON_SCALE, 0.0, INT32_MIN, INT32_MAX);
    out->lon = quantize_clamped(data->gps.lon, LATLON_SCALE, 0.0, INT32_MIN, INT32_MAX);
    out->altitude_gps_m = (int16_t)quantize_clamped(data->gps.alt_m, ALT_GPS_SCALE, 0.0, INT16_MIN, INT16_MAX);

    {
        uint8_t sats = data->gps.sats;
        uint8_t fix = data->gps.fix_type;

        if (sats > SATFIX_SATS_MAX)
            sats = SATFIX_SATS_MAX;
        if (fix > SATFIX_FIX_MASK)
            fix = SATFIX_FIX_MASK;
        out->satfix = (uint8_t)((sats << SATFIX_FIX_BITS) | fix);
    }

    /* ---------- battery ---------- */
    out->vbatt = (uint8_t)quantize_clamped(data->battery.voltage, VBATT_SCALE, VBATT_OFFSET, 0, UINT8_MAX);

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

lora_decode_result_t lora_decode(lora_decoded_t *out, const uint8_t *buf, size_t len)
{
    data_to_lora_t in;

    if (len != sizeof(in))
        return LORA_DECODE_BAD_LENGTH;

    /* Running the CRC over the data AND its crc16 gives 0 if nothing got corrupted */
    if (crc16(buf, len) != 0)
        return LORA_DECODE_BAD_CRC;

    memcpy(&in, buf, sizeof(in)); /* buf may be unaligned, the copy is safe to read field by field */

    if (in.sync != SYNC_WORD)
        return LORA_DECODE_BAD_SYNC;
    if (in.version != LORA_VERSION)
        return LORA_DECODE_BAD_VERSION;

    memset(out, 0, sizeof(*out));

    /* ---------- header ---------- */
    out->version = in.version;
    out->seq = in.seq;

    /* ---------- flight info ---------- */
    out->ms_since_boot = in.ms_since_boot;
    out->flight_state = in.flight_state;
    out->records_dropped = in.records_dropped;

    /* ---------- environment ---------- */
    out->temp_c = (float)dequantize(in.temp_c, TEMP_SCALE, 0.0);
    out->pressure_pa = (float)dequantize(in.pressure, PRESSURE_SCALE, PRESSURE_OFFSET);
    out->humidity_pct = (float)dequantize(in.humidity_pct, HUMIDITY_SCALE, 0.0);
    out->altitude_baro_m = (float)dequantize(in.altitude_baro_m, ALT_BARO_SCALE, 0.0);

    /* ---------- IMU ---------- */
    for (int i = 0; i < 3; i++)
    {
        out->accel[i] = (float)dequantize(in.accel[i], ACCEL_SCALE, 0.0);
        out->gyro[i] = (float)dequantize(in.gyro[i], GYRO_SCALE, 0.0);
    }
    out->accel_mag = (float)dequantize(in.accel_mag, ACCEL_SCALE, 0.0);

    /* ---------- light sensors ---------- */
    out->is_there_light_uv = in.is_there_light_uv != 0;
    out->is_there_light_fotores = in.is_there_light_fotores != 0;
    out->uv_index = (float)dequantize(in.uv, UV_SCALE, 0.0);

    /* ---------- gas sensor ---------- */
    out->gas_raw = in.gas_raw;
    out->heater_on_s = in.heater_on_since;

    /* ---------- GPS ---------- */
    out->lat = dequantize(in.lat, LATLON_SCALE, 0.0);
    out->lon = dequantize(in.lon, LATLON_SCALE, 0.0);
    out->altitude_gps_m = (float)dequantize(in.altitude_gps_m, ALT_GPS_SCALE, 0.0);
    out->sats = (uint8_t)(in.satfix >> SATFIX_FIX_BITS);
    out->fix_type = (uint8_t)(in.satfix & SATFIX_FIX_MASK);

    /* ---------- battery ---------- */
    out->vbatt = (float)dequantize(in.vbatt, VBATT_SCALE, VBATT_OFFSET);

    /* ---------- status bits ---------- */
    out->status = in.status;

    return LORA_DECODE_OK;
}
