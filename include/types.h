#ifndef TYPES_H
#define TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "config.h"

#pragma region main_datastore

    /*
    WHEN UPDATING THESE REMEMBER TO UPDATE ALSO THE PACKED VERSIONS BELOW
    */

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
        float accel[3]; // 0 = x, 1 = y, 2 = z. measured in g's
        float gyro[3];  // same as above. measured in degrees per second
        float accel_mag;
    } imu_t;

    typedef struct
    {
        stamp_t s;
        double lat, lon;
        float alt_m, speed_ms, course_deg, hdop; /* HDOP is an accuracy indicator */
        uint8_t sats, fix_type;
        uint32_t utc_s; /* for aligning logs */
    } gps_t;

    typedef struct
    {
        stamp_t s;
        bool is_there_light;
        uint16_t uv_raw; /* averaged ADC counts */
        float uv_index;  /* converted           */
    } uv_t;

    typedef struct
    {
        stamp_t s;
        bool is_there_light;
    } fotores_t;

    typedef struct
    {
        stamp_t s;
        uint16_t raw;
        uint16_t heater_on_s; /* heater on timestamp needed to consider it in error correction */
        bool heater_enabled;
    } gas_t;

    typedef struct
    {
        stamp_t s;
        uint16_t voltage_raw; // raw ADC counts
        float voltage;
        float percentage;
        bool is_alert_active;
    } battery_t;

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
        FLIGHT_STATE_BOOT = 0,
        FLIGHT_STATE_SELF_TEST = 1,
        FLIGHT_STATE_IDLE = 2,
        FLIGHT_STATE_ASCENT = 3,
        FLIGHT_STATE_DESCENT = 4,
        FLIGHT_STATE_LANDED = 5
    } flight_state_t;

    typedef struct
    {
        uint32_t t_ms; /* time of last update, any block */
        flight_state_t state;
        // uint32_t t_state_entry; /* for timeouts in the FSM (no idea what this means lol) */
        float alt_peak_env, alt_peak_gps;
        uint16_t records_dropped; /* records dropped due to ring buffer having not enough space*/

        env_t env;
        imu_t imu;
        gps_t gps;
        uv_t uv;
        fotores_t fotores;
        gas_t gas;
        // health_t health;
    } datastore_t;

#pragma endregion

#pragma region lora_structs
/*
Note that floats are converted to integer to make each field have fixed size.
For now the choice of the length of such integer has been made by Claude, we will need to adjust them.
*/
#pragma pack(push, 1)
    typedef struct
    {
        uint16_t sync; /* MUST TO BE FIRST! MUST ALWAYS BE SYNC_WORD. Unusual pattern needed to detect packet loss. */
        uint8_t version;
        uint16_t seq; /* needed for packet loss statistics (details needed for implementation) */

        uint32_t ms_since_boot;
        uint8_t flight_state;
        uint16_t records_dropped; /* records dropped due to ring buffer having not enough space*/

        int16_t temp_c;
        uint16_t pressure; /* (pa - 30000) / 2) */
        uint8_t humidity_pct;
        int16_t altitude_baro_m;

        int16_t orientation[4];
        int16_t accel_mag;

        uint8_t is_there_light_uv; /* uint_8 instead of bool because bool has unspecified size */
        uint8_t is_there_light_fotores;
        uint8_t uv;

        uint16_t gas_raw;
        uint8_t heater_on_since;

        int32_t lat, lon;
        int16_t altitude_gps_m;
        uint8_t satfix;

        uint8_t vbatt;
        uint16_t status;

        uint16_t crc16; /* MUST BE LAST! */
    } data_to_lora_t;
#pragma pack(pop)

    static_assert(sizeof(data_to_lora_t) == 51, "LoRa layout changed");

#pragma endregion

#pragma region sd_structs

    /*
    WHEN UPDATING THESE REMEMBER TO UPDATE ALSO THE NOT PACKED VERSIONS ABOVE
    */

    typedef enum
    {
        DATA_TYPE_ENV = 0,
        DATA_TYPE_IMU = 1,
        DATA_TYPE_GPS = 2,
        DATA_TYPE_UV = 3,
        DATA_TYPE_FOTORES = 4,
        DATA_TYPE_GAS = 5,
        DATA_TYPE_STATS = 6,
        // HEALTH = 7,
    } data_type_t;

#pragma pack(push, 1)

    typedef struct
    {
        uint16_t sync; /* MUST TO BE FIRST! MUST ALWAYS BE SYNC_WORD. Unusual pattern needed to detect packet loss. */
        uint8_t type;
        uint16_t len;
        uint32_t t_ms;
    } sd_header_t;

    // Packed versions of sensor structs
    typedef struct
    {
        uint32_t t_ms; /* when this block was last updated */
        uint8_t valid; /* did the last read succeed?       */
    } stamp_packed_t;

    typedef struct
    {
        stamp_packed_t s;
        float temp_c;
        float pressure_pa;
        float humidity_pct;
        float altitude_m; /* derived, relative to the pad */
    } env_packed_t;

    typedef struct
    {
        stamp_packed_t s;
        float accel[3];
        float gyro[3];
        float accel_mag;
    } imu_packed_t;

    typedef struct
    {
        stamp_packed_t s;
        double lat, lon;
        float alt_m, speed_ms, course_deg, hdop; /* HDOP is an accuracy indicator */
        uint8_t sats, fix_type;
        uint32_t utc_s; /* for aligning logs */
    } gps_packed_t;

    typedef struct
    {
        stamp_packed_t s;
        uint8_t is_there_light;
        uint16_t uv_raw; /* averaged ADC counts */
        float uv_index;  /* converted           */
    } uv_packed_t;

    typedef struct
    {
        stamp_packed_t s;
        uint8_t is_there_light;
    } fotores_packed_t;

    typedef struct
    {
        stamp_packed_t s;
        uint16_t raw;
        uint16_t heater_on_s; /* heater on timestamp needed to consider it in error correction */
        uint8_t heater_enabled;
    } gas_packed_t;

    typedef struct
    {
        uint32_t t_ms; /* time of last update, any block */
        uint8_t state;
        float alt_peak_env;
        float alt_peak_gps;
        uint16_t records_dropped; /* records dropped due to ring buffer having not enough space*/
    } stats_packed_t;

    // Structs with header, data and crc for direct sd write
    typedef struct
    {
        sd_header_t header;
        env_packed_t data;
        uint16_t crc;
    } env_sd_t;

    typedef struct
    {
        sd_header_t header;
        imu_packed_t data;
        uint16_t crc;
    } imu_sd_t;

    typedef struct
    {
        sd_header_t header;
        gps_packed_t data;
        uint16_t crc;
    } gps_sd_t;

    typedef struct
    {
        sd_header_t header;
        uv_packed_t data;
        uint16_t crc;
    } uv_sd_t;

    typedef struct
    {
        sd_header_t header;
        fotores_packed_t data;
        uint16_t crc;
    } fotores_sd_t;

    typedef struct
    {
        sd_header_t header;
        gas_packed_t data;
        uint16_t crc;
    } gas_sd_t;

    typedef struct
    {
        sd_header_t header;
        stats_packed_t data;
        uint16_t crc;
    } stats_sd_t;

#pragma pack(pop)

    static_assert(sizeof(sd_header_t) == 9, "Header layout changed");
    static_assert(sizeof(stamp_packed_t) == 5, "Stamp packed layout changed");
    static_assert(sizeof(env_packed_t) == 21, "Env packed layout changed");
    static_assert(sizeof(imu_packed_t) == 33, "IMU packed layout changed");
    static_assert(sizeof(gps_packed_t) == 43, "GPS packed layout changed");
    static_assert(sizeof(uv_packed_t) == 12, "UV packed layout changed");
    static_assert(sizeof(fotores_packed_t) == 6, "Fotores packed layout changed");
    static_assert(sizeof(gas_packed_t) == 10, "Gas packed layout changed");
    static_assert(sizeof(stats_packed_t) == 15, "Stats packed layout changed");

#pragma endregion

#pragma region sensor_states

    // internal states
    typedef enum
    {
        SENS_IDLE,
        SENS_WAITING,
        SENS_OK,
    } sens_state_t;

    // external status
    typedef enum
    {
        SENS_NOT_READY,
        SENS_READY,
        SENS_ERR,
    } sens_status_t;

#pragma endregion

#pragma region scheduler

    /*
    Tasks (so that I can define the friggin structure):
    - sensor reads
    - phase change check
    - (watchdog?)
    */

    typedef struct task_s task_t;
    struct task_s
    {
        uint32_t (*run)(task_t *self, uint32_t now_millis);
        void (*on_enable)(task_t *self);  /* NULL if nothing to do */
        void (*on_disable)(task_t *self); /* NULL if nothing to do */
        sens_status_t last_status;        // TODO: eventually replace with general task status and wrap the sens_status
        const char *name;
        uint16_t period;
        uint32_t due;
        uint8_t priority; // The lower the number, the higher the priority
    };

    typedef struct
    {
        task_t *tasks;
        size_t tasks_number;
        uint16_t max_exec_time;
        uint32_t run_count;
        bool *enabled_tasks;
    } scheduler_t;

#pragma endregion

#pragma region colors

    typedef struct
    {
        uint8_t red, green, blue;
    } rgb_colors_t;

#define RED ((rgb_colors_t){255, 0, 0})
#define GREEN ((rgb_colors_t){0, 255, 0})
#define BLUE ((rgb_colors_t){0, 0, 255})
#define YELLOW ((rgb_colors_t){255, 255, 0})
#define VIOLET ((rgb_colors_t){255, 0, 255})

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif