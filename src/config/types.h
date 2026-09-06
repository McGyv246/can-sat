#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t t_ms; /* when this block was last updated */
    bool valid;    /* did the last read succeed?       */
} stamp_t;

typedef struct
{
    stamp_t s;
    float temp_c;
    float pressure_pa;
    float humidity_pct;
    float altitude_m; /* derived, relative to the pad */
} env_t;

typedef struct
{
    stamp_t s;
    float accel[3];
    float gyro[3];
    float mag[3];
    float quat[4];   /* automatically fused by the sensor dedicated processor */
    float accel_mag; /* not sure if useful */
} imu_t;

typedef struct
{
    stamp_t s;
    double lat, lon;
    float alt_m, speed_ms, course_deg, hdop; /* HDOP is an accuracy indicator */
    uint8_t sats, fix_type;
    uint32_t utc_s; /* for aligning logs */
} gps_t;

// Bisognerà capire se questo va bene così o no in base a come è stato collegato (digitale?)
typedef struct
{
    stamp_t s;
    uint16_t lux_raw, uv_raw; /* averaged ADC counts */
    float lux, uv_index;      /* converted           */
} light_t;

typedef struct
{
    stamp_t s;
    uint16_t raw;
    uint16_t heater_on_s; /* heater on timestamp needed to consider it in error correction */
    bool heater_enabled;
} gas_t;

// typedef struct
// {
//     stamp_t s;
//     float vbatt;
//     uint16_t status_bits; /* one bit per subsystem */
//     uint16_t log_dropped, tx_dropped;
//     uint8_t cpu_load_pct;
// } health_t;

typedef enum
{
    BOOT,
    SELF_TEST,
    IDLE,
    ASCENT,
    DESCENT,
    LANDED
} flight_state_t;

typedef struct
{
    uint32_t t_ms; /* time of last update, any block */
    flight_state_t state;
    uint32_t t_state_entry; /* for timeouts in the FSM */
    float alt_peak;

    env_t env;
    imu_t imu;
    gps_t gps;
    light_t light;
    gas_t gas;
    // health_t health;
} datastore_t;

#endif