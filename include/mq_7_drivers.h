#ifndef MQ_7_DRIVERS_H
#define MQ_7_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void mq_7_init(void);

    void mq_7_wake(uint32_t now_ms);

    sens_state_t mq_7_poll(uint32_t now_ms);

    sens_state_t mq_7_get(gas_t *data);

    void mq_7_sleep(void);

#ifdef __cplusplus
}
#endif

#endif