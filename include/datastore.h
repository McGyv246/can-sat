#ifndef DATASTORE_H
#define DATASTORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void ds_write_env(env_t *data, uint32_t now_millis);
    void ds_write_imu(imu_t *data, uint32_t now_millis);
    void ds_write_gps(gps_t *data, uint32_t now_millis);
    void ds_write_uv(uv_t *data, uint32_t now_millis);
    void ds_write_fotores(fotores_t *data, uint32_t now_millis);
    void ds_write_gas(gas_t *data, uint32_t now_millis);
    void ds_write_battery(battery_t *data, uint32_t now_millis);
    void ds_write_thermal(thermal_summary_t *data, uint32_t now_millis);
    void ds_write_flight_state(flight_state_t data, uint32_t now_millis);
    void ds_write_increment_records_dropped(uint32_t now_millis);

    bool ds_read_all(datastore_t *data);
    flight_state_t ds_read_state(void);

    // ONLY for testing purposes
    void _ds_set_full(datastore_t *new_ds);

#ifdef __cplusplus
}
#endif

#endif