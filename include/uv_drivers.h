#ifndef UV_DRIVERS_H_
#define UV_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    // this function is to be called on startup to set the pins
    sens_external_state_t uv_init(void);

    sens_external_state_t uv_poll(uint32_t now_ms);

    sens_external_state_t uv_get(uv_t *data);

    // there is no implemented way of turning on/off this sensor
    // sens_external_state_t sleep(void);

#ifdef __cplusplus
}
#endif

#endif