// compile: gcc -fsanitize=address -I include -I mocks -Wall src/codec/pack_data_sd.c src/codec/crc.c src/service/datastore.c src/service/ring_buffer.c mocks/hal_time.c mocks/gen_bin_data_file.c -lm -o build/gen_bin_data

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "pack_data_sd.h"
#include "ring_buffer.h"
#include "config.h"

datastore_t store = {
    .t_ms = 45230,
    .state = FLIGHT_STATE_ASCENT,
    .alt_peak_env = 312.4f,
    .alt_peak_gps = 309.8f,
    .records_dropped = 0,

    .env = {
        .s = {.t_ms = 45220, .valid = true},
        .temp_c = 17.8f,
        .pressure_pa = 97650.0f,
        .humidity_pct = 54.2f,
        .altitude_m = 312.4f,
    },

    .imu = {
        .s = {.t_ms = 45228, .valid = true},
        .accel = {0.42f, -0.18f, 24.7f}, /* m/s^2, thrust along z */
        .gyro = {0.03f, -0.05f, 1.21f},  /* rad/s */
        .accel_mag = 24.71f,
    },

    .gps = {
        .s = {.t_ms = 45100, .valid = true},
        .lat = 45.6983, /* near Bergamo */
        .lon = 9.6773,
        .alt_m = 309.8f,
        .speed_ms = 68.4f,
        .course_deg = 214.5f,
        .hdop = 0.9f,
        .sats = 11,
        .fix_type = 3,       /* 3D fix */
        .utc_s = 1790413245, /* 2026-09-26 08:00:45 UTC */
    },

    .uv = {
        .s = {.t_ms = 45210, .valid = true},
        .is_there_light = true,
        .uv_raw = 512,
        .uv_index = 3.7f,
    },

    .fotores = {
        .s = {.t_ms = 45220, .valid = true},
        .is_there_light = true,
    },

    .gas = {
        .s = {.t_ms = 45150, .valid = true},
        .raw = 18432,
        .heater_on_s = 32,
        .heater_enabled = true,
    },

    .battery = {
        .s = {.t_ms = 45230, .valid = true},
        .voltage_raw = 12564,
        .voltage = 3.25,
        .percentage = 0.67,
        .is_alert_active = false,
    },
};

// Not in the datastore (it only has the summary): filled in main with a gradient
static thermal_t thermal_frame = {
    .s = {.t_ms = 45200, .valid = true},
};

static int dump_to_file(const char *path, const uint8_t *data, size_t len)
{
    FILE *f = fopen(path, "wb"); /* "wb" matters on Windows: no newline translation */
    if (!f)
    {
        perror("fopen");
        return -1;
    }
    size_t n = fwrite(data, 1, len, f);
    fclose(f);
    return (n == len) ? 0 : -1;
}

/* Read the whole file into a malloc'd buffer. Returns NULL on failure. */
static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror("fopen");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(size);
    if (buf && fread(buf, 1, size, f) != (size_t)size)
    {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    *len = (size_t)size;
    return buf;
}

/* Copy the next record into dst, checking bounds, sync, type and length. */
static int take(const uint8_t *buf, size_t buf_len, size_t *off,
                void *dst, size_t rec_size, uint8_t expected_type)
{
    if (*off + rec_size > buf_len)
    {
        fprintf(stderr, "truncated record (type %u) at offset %zu\n", expected_type, *off);
        return -1;
    }
    memcpy(dst, buf + *off, rec_size);

    const sd_header_t *h = (const sd_header_t *)dst;
    if (h->sync != SYNC_WORD || h->type != expected_type ||
        h->len != rec_size - sizeof(sd_header_t) - sizeof(uint16_t))
    {
        fprintf(stderr, "bad header for type %u at offset %zu\n", expected_type, *off);
        return -1;
    }
    *off += rec_size;
    return 0;
}

int main()
{
    // WRITE
    env_sd_t env;
    imu_sd_t imu;
    gps_sd_t gps;
    uv_sd_t uv;
    fotores_sd_t fotores;
    gas_sd_t gas;
    battery_sd_t battery;
    stats_sd_t stats;
    static thermal_sd_t thermal; // static: 1.5 KB
    for (size_t i = 0; i < THERMAL_PIXELS; i++)
        thermal_frame.pixels[i] = 15.0f + (float)i * 0.05f;
    env_to_sd(&env, &store.env);
    imu_to_sd(&imu, &store.imu);
    gps_to_sd(&gps, &store.gps);
    uv_to_sd(&uv, &store.uv);
    fotores_to_sd(&fotores, &store.fotores);
    gas_to_sd(&gas, &store.gas);
    battery_to_sd(&battery, &store.battery);
    stats_to_sd(&stats, &store);
    thermal_to_sd(&thermal, &thermal_frame);

    buf_write(&env, sizeof(env_sd_t));
    buf_write(&imu, sizeof(imu_sd_t));
    buf_write(&gps, sizeof(gps_sd_t));
    buf_write(&uv, sizeof(uv_sd_t));
    buf_write(&fotores, sizeof(fotores_sd_t));
    buf_write(&gas, sizeof(gas_sd_t));
    buf_write(&battery, sizeof(battery_sd_t));
    buf_write(&stats, sizeof(stats_sd_t));
    buf_write(&thermal, sizeof(thermal_sd_t));

    static uint8_t out[BUF_SIZE];
    size_t len = buf_read(out);

    if (dump_to_file("data.bin", out, len) != 0)
    {
        fprintf(stderr, "failed to write file\n");
        return 1;
    }
    printf("wrote %zu bytes to data.bin\n", len);

    // READ
    size_t rlen = 0;
    uint8_t *rbuf = read_file("data.bin", &rlen);
    if (!rbuf)
        return 1;

    env_sd_t r_env;
    imu_sd_t r_imu;
    gps_sd_t r_gps;
    uv_sd_t r_uv;
    fotores_sd_t r_fotores;
    gas_sd_t r_gas;
    battery_sd_t r_battery;
    stats_sd_t r_stats;
    static thermal_sd_t r_thermal;

    size_t off = 0;
    if (take(rbuf, rlen, &off, &r_env, sizeof r_env, DATA_TYPE_ENV) ||
        take(rbuf, rlen, &off, &r_imu, sizeof r_imu, DATA_TYPE_IMU) ||
        take(rbuf, rlen, &off, &r_gps, sizeof r_gps, DATA_TYPE_GPS) ||
        take(rbuf, rlen, &off, &r_uv, sizeof r_uv, DATA_TYPE_UV) ||
        take(rbuf, rlen, &off, &r_fotores, sizeof r_fotores, DATA_TYPE_FOTORES) ||
        take(rbuf, rlen, &off, &r_gas, sizeof r_gas, DATA_TYPE_GAS) ||
        take(rbuf, rlen, &off, &r_battery, sizeof r_battery, DATA_TYPE_BATTERY) ||
        take(rbuf, rlen, &off, &r_stats, sizeof r_stats, DATA_TYPE_STATS) ||
        take(rbuf, rlen, &off, &r_thermal, sizeof r_thermal, DATA_TYPE_THERMAL))
    {
        free(rbuf);
        return 1;
    }
    free(rbuf);

    int errors = 0;
#define CHECK(cond)                                       \
    do                                                    \
    {                                                     \
        if (!(cond))                                      \
        {                                                 \
            fprintf(stderr, "CHECK FAILED: %s\n", #cond); \
            errors++;                                     \
        }                                                 \
    } while (0)

    /* whole file consumed, nothing left over */
    CHECK(rlen == len);
    CHECK(off == rlen);

    /* whole records (header + data + crc) identical to what was written */
    CHECK(memcmp(&r_env, &env, sizeof env) == 0);
    CHECK(memcmp(&r_imu, &imu, sizeof imu) == 0);
    CHECK(memcmp(&r_gps, &gps, sizeof gps) == 0);
    CHECK(memcmp(&r_uv, &uv, sizeof uv) == 0);
    CHECK(memcmp(&r_fotores, &fotores, sizeof fotores) == 0);
    CHECK(memcmp(&r_gas, &gas, sizeof gas) == 0);
    CHECK(memcmp(&r_battery, &battery, sizeof battery) == 0);
    CHECK(memcmp(&r_stats, &stats, sizeof stats) == 0);
    CHECK(memcmp(&r_thermal, &thermal, sizeof thermal) == 0);

    /* spot checks against the original datastore */
    CHECK(r_env.data.s.t_ms == store.env.s.t_ms);
    CHECK(r_env.data.s.valid == store.env.s.valid);
    CHECK(r_env.data.temp_c == store.env.temp_c);
    CHECK(r_env.data.pressure_pa == store.env.pressure_pa);
    CHECK(r_env.data.altitude_m == store.env.altitude_m);

    CHECK(r_imu.data.accel[2] == store.imu.accel[2]);
    CHECK(r_imu.data.accel_mag == store.imu.accel_mag);

    CHECK(r_gps.data.lat == store.gps.lat);
    CHECK(r_gps.data.lon == store.gps.lon);
    CHECK(r_gps.data.sats == store.gps.sats);
    CHECK(r_gps.data.utc_s == store.gps.utc_s);

    CHECK(r_uv.data.is_there_light == store.uv.is_there_light);
    CHECK(r_uv.data.uv_raw == store.uv.uv_raw);
    CHECK(r_uv.data.uv_index == store.uv.uv_index);

    CHECK(r_fotores.data.is_there_light == store.fotores.is_there_light);

    CHECK(r_gas.data.raw == store.gas.raw);
    CHECK(r_gas.data.heater_on_s == store.gas.heater_on_s);
    CHECK(r_gas.data.heater_enabled == store.gas.heater_enabled);

    CHECK(r_battery.data.voltage_raw == store.battery.voltage_raw);
    CHECK(r_battery.data.voltage == store.battery.voltage);
    CHECK(r_battery.data.percentage == store.battery.percentage);
    CHECK(r_battery.data.is_alert_active == store.battery.is_alert_active);

    CHECK(r_stats.data.t_ms == store.t_ms);
    CHECK(r_stats.data.state == (uint8_t)store.state);
    CHECK(r_stats.data.alt_peak_env == store.alt_peak_env);
    CHECK(r_stats.data.alt_peak_gps == store.alt_peak_gps);
    CHECK(r_stats.data.records_dropped == store.records_dropped);

    CHECK(r_thermal.data.s.t_ms == thermal_frame.s.t_ms);
    CHECK(r_thermal.data.pixels[0] == 1500);                      /* 15.00 C */
    CHECK(r_thermal.data.pixels[THERMAL_PIXELS - 1] == 5335);     /* 15 + 767 * 0.05 = 53.35 C */
#undef CHECK

    if (errors == 0)
        printf("parse OK: all %d records match\n", 9);
    else
        printf("parse finished with %d failed checks\n", errors);

    return errors ? 1 : 0;

    return 0;
}