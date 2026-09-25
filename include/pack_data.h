#ifndef PACK_DATA_H
#define PACK_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void env_to_sd(env_sd_t *out, env_t *data);
    void imu_to_sd(imu_sd_t *out, imu_t *data);
    void gps_to_sd(gps_sd_t *out, gps_t *data);
    void light_to_sd(light_sd_t *out, light_t *data);
    void gas_to_sd(gas_sd_t *out, gas_t *data);
    void stats_to_sd(stats_sd_t *out, datastore_t *data);

#ifdef __cplusplus
}
#endif

#endif