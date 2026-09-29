#ifndef LORA_DRIVERS_H_
#define LORA_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include "types.h"

    // callback function. should NOT be called anywhere EVER
    void onTxDone(void);

    sens_external_state_t lora_init(void);

    void lora_wake(void);

    // sends packets in a non-blocking way. returns "SENS_EXTERNAL_NOT_READY" if the module is busy sending a packet or duty cycle
    // standby time hasn't expired yet
    sens_external_state_t lora_send_packet(uint8_t buffer[], size_t len);

    void lora_sleep(void);

#ifdef __cplusplus
}
#endif

#endif