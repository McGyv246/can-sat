#ifndef FOTORES_DRIVERS_H
#define FOTORES_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    // this function is to be called once on setup to set the pins
    void fotores_init(void);

    sens_state_t fotores_poll(uint32_t now_ms);

    void fotores_get(fotores_t *data);

#ifdef __cplusplus
}
#endif

#endif