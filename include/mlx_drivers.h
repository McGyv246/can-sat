#ifndef MLX_DRIVERS_H
#define MLX_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    sens_state_t mlx_init(void);

    sens_state_t mlx_poll(uint32_t now_ms);

    void mlx_get(thermal_t *data);

#ifdef __cplusplus
}
#endif

#endif