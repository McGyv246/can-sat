// compile: gcc -fsanitize=address -I include -I mocks -Wall src/codec/pack_data.c src/codec/crc.c src/service/datastore.c src/service/ring_buffer.c mocks/hal_time.c mocks/gen_bin_data_file.c -o build/gen_bin_data

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "pack_data.h"
#include "ring_buffer.h"
#include "config.h"

datastore_t store = {
    .t_ms = 45230,
    .state = ASCENT,
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
        .mag = {22.5f, -4.3f, 41.8f},    /* uT */
        .quat = {0.9962f, 0.0348f, -0.0523f, 0.0523f},
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

    .light = {
        .s = {.t_ms = 45210, .valid = true},
        .is_there_light = true,
        .uv_raw = 512,
        .uv_index = 3.7f,
    },

    .gas = {
        .s = {.t_ms = 45150, .valid = true},
        .raw = 18432,
        .heater_on_s = 32,
        .heater_enabled = true,
    },
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
    light_sd_t light;
    gas_sd_t gas;
    stats_sd_t stats;
    env_to_sd(&env, &store.env);
    imu_to_sd(&imu, &store.imu);
    gps_to_sd(&gps, &store.gps);
    light_to_sd(&light, &store.light);
    gas_to_sd(&gas, &store.gas);
    stats_to_sd(&stats, &store);

    buf_write(&env, sizeof(env_sd_t));
    buf_write(&imu, sizeof(imu_sd_t));
    buf_write(&gps, sizeof(gps_sd_t));
    buf_write(&light, sizeof(light_sd_t));
    buf_write(&gas, sizeof(gas_sd_t));
    buf_write(&stats, sizeof(stats_sd_t));

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
    light_sd_t r_light;
    gas_sd_t r_gas;
    stats_sd_t r_stats;

    size_t off = 0;
    if (take(rbuf, rlen, &off, &r_env, sizeof r_env, ENV) ||
        take(rbuf, rlen, &off, &r_imu, sizeof r_imu, IMU) ||
        take(rbuf, rlen, &off, &r_gps, sizeof r_gps, GPS) ||
        take(rbuf, rlen, &off, &r_light, sizeof r_light, LIGHT) ||
        take(rbuf, rlen, &off, &r_gas, sizeof r_gas, GAS) ||
        take(rbuf, rlen, &off, &r_stats, sizeof r_stats, STATS))
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
    CHECK(memcmp(&r_light, &light, sizeof light) == 0);
    CHECK(memcmp(&r_gas, &gas, sizeof gas) == 0);
    CHECK(memcmp(&r_stats, &stats, sizeof stats) == 0);

    /* spot checks against the original datastore */
    CHECK(r_env.data.s.t_ms == store.env.s.t_ms);
    CHECK(r_env.data.s.valid == store.env.s.valid);
    CHECK(r_env.data.temp_c == store.env.temp_c);
    CHECK(r_env.data.pressure_pa == store.env.pressure_pa);
    CHECK(r_env.data.altitude_m == store.env.altitude_m);

    CHECK(r_imu.data.accel[2] == store.imu.accel[2]);
    CHECK(r_imu.data.quat[0] == store.imu.quat[0]);
    CHECK(r_imu.data.accel_mag == store.imu.accel_mag);

    CHECK(r_gps.data.lat == store.gps.lat);
    CHECK(r_gps.data.lon == store.gps.lon);
    CHECK(r_gps.data.sats == store.gps.sats);
    CHECK(r_gps.data.utc_s == store.gps.utc_s);

    CHECK(r_light.data.is_there_light == store.light.is_there_light);
    CHECK(r_light.data.uv_raw == store.light.uv_raw);
    CHECK(r_light.data.uv_index == store.light.uv_index);

    CHECK(r_gas.data.raw == store.gas.raw);
    CHECK(r_gas.data.heater_on_s == store.gas.heater_on_s);
    CHECK(r_gas.data.heater_enabled == store.gas.heater_enabled);

    CHECK(r_stats.data.t_ms == store.t_ms);
    CHECK(r_stats.data.state == (uint8_t)store.state);
    CHECK(r_stats.data.alt_peak_env == store.alt_peak_env);
    CHECK(r_stats.data.alt_peak_gps == store.alt_peak_gps);
    CHECK(r_stats.data.records_dropped == store.records_dropped);
#undef CHECK

    if (errors == 0)
        printf("parse OK: all %d records match\n", 6);
    else
        printf("parse finished with %d failed checks\n", errors);

    return errors ? 1 : 0;

    return 0;
}