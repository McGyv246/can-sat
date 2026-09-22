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
    void ds_write_light(light_t *data, uint32_t now_millis);
    void ds_write_gas(gas_t *data, uint32_t now_millis);
    void ds_write_flight_state(flight_state_t data, uint32_t now_millis);

    bool ds_read_all(datastore_t *data);

#ifdef __cplusplus
}
#endif

#endif