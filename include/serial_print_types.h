#ifndef SERIAL_PRINT_TYPES_H
#define SERIAL_PRINT_TYPES_H

/*
Debug helpers that print every type defined in types.h to the Arduino Serial port
in a human readable, indented format. Serial must already be initialized (Serial.begin).
All functions accept NULL pointers and print "(null)" in that case.
*/

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma region main_datastore

    void print_stamp(const stamp_t *x);
    void print_env(const env_t *x);
    void print_imu(const imu_t *x);
    void print_gps(const gps_t *x);
    void print_uv(const uv_t *x);
    void print_fotores(const fotores_t *x);
    void print_gas(const gas_t *x);
    void print_battery(const battery_t *x);
    void print_flight_state(flight_state_t x);
    void print_datastore(const datastore_t *x);

#pragma endregion

#pragma region lora_structs

    void print_data_to_lora(const data_to_lora_t *x);

#pragma endregion

#pragma region sd_structs

    void print_data_type(data_type_t x);
    void print_sd_header(const sd_header_t *x);

    void print_stamp_packed(const stamp_packed_t *x);
    void print_env_packed(const env_packed_t *x);
    void print_imu_packed(const imu_packed_t *x);
    void print_gps_packed(const gps_packed_t *x);
    void print_uv_packed(const uv_packed_t *x);
    void print_fotores_packed(const fotores_packed_t *x);
    void print_gas_packed(const gas_packed_t *x);
    void print_battery_packed(const battery_packed_t *x);
    void print_stats_packed(const stats_packed_t *x);

    void print_env_sd(const env_sd_t *x);
    void print_imu_sd(const imu_sd_t *x);
    void print_gps_sd(const gps_sd_t *x);
    void print_uv_sd(const uv_sd_t *x);
    void print_fotores_sd(const fotores_sd_t *x);
    void print_gas_sd(const gas_sd_t *x);
    void print_battery_sd(const battery_sd_t *x);
    void print_stats_sd(const stats_sd_t *x);

#pragma endregion

#pragma region sensor_states

    void print_sens_state(sens_state_t x);
    void print_task_state(task_state_t x);

#pragma endregion

#pragma region scheduler

    void print_task(const task_t *x);
    void print_scheduler(const scheduler_t *x);

#pragma endregion

#pragma region colors

    void print_rgb_colors(const rgb_colors_t *x);

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif
