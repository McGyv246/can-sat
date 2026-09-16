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

#pragma region main_datastore

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
        float quat[4]; /* automatically fused by the sensor dedicated processor */
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
        BOOT = 0,
        SELF_TEST = 1,
        IDLE = 2,
        ASCENT = 3,
        DESCENT = 4,
        LANDED = 5
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

#pragma endregion

#pragma region lora_structs
/*
Note that floats are converted to integer to make each field have fixed size.
For now the choice of the length of such integer has been made by Claude, we will need to adjust them.
*/
#pragma pack(push, 1)
    typedef struct
    {
        uint8_t sync; /* MUST TO BE FIRST! 0xA5A5 (don't know what it means lol). Needed to handle packet loss (details needed for implementation) */
        uint8_t version;
        uint16_t seq; /* needed for packet loss statistics (details needed for implementation) */

        uint32_t ms_since_boot;
        uint8_t flight_state;

        int16_t temp_c;
        uint16_t pressure; /* (pa - 30000) / 2) */
        uint8_t humidity_pct;
        int16_t altitude_baro_m;

        int16_t orientation[4];
        int16_t accel_mag;

        uint8_t is_there_light; /* uint_8 instead of bool because bool has unspecified size */
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

    static_assert(sizeof(data_to_lora_t) == 47, "wire layout changed");

#pragma endregion

#pragma region sd_structs

    typedef enum
    {
        ENV = 0,
        IMU = 1,
        GPS = 2,
        LIGHT = 3,
        GAS = 4,
        // HEALTH = 5
    } sensor_type_t;

#pragma pack(push, 1)
    typedef struct
    {
        uint8_t sync; /* MUST TO BE FIRST! 0xA5A5 (don't know what it means lol). Needed to handle packet loss (details needed for implementation) */
        uint8_t type;
        uint16_t len;
        uint32_t t_ms;
    } sd_header_t;
#pragma pack(pop)

    static_assert(sizeof(sd_header_t) == 8, "wire layout changed");

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
        -
        */
    typedef struct
    {
        // This structure is correct for sensors, but it could create problems if I wanted to put some different tasks to run
        // Maybe I could just wrap sensors in some sort of runnable thing, but I don't know how I could get the sleep and init functions,
        // while poll and get can be easily wrapped in a function that checks the return status and then gets automatically the value.
        uint32_t (*run)(void);
        void (*on_enable)(void);  /* NULL if nothing to do */
        void (*on_disable)(void); /* NULL if nothing to do */
        sens_status_t last_status;
        const char *name;
        uint16_t period;
        uint64_t due;
        uint8_t priority; // The lower the number, the higher the priority
    } task_t;

    typedef struct
    {
        task_t *tasks;
        size_t tasks_number;
        uint16_t max_exec_time;
        uint64_t run_count;
        bool *enabled_tasks;
    } scheduler_t;

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif