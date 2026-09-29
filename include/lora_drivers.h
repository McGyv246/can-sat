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

    // callback function. should NOT be called anywhere EVER
    void onReceive(int packetSize);

    // initializes as receiver if parameter is true
    sens_status_t lora_init(bool is_receiver);

    void lora_wake(void);

    // sends packets in a non-blocking way. returns "SENS_NOT_READY" if the module is busy sending a packet or duty cycle
    // standby time hasn't expired yet
    sens_status_t lora_send_packet(uint8_t buffer[], size_t len);

    sens_status_t lora_read_packet(uint8_t *buffer, size_t len);

    void lora_sleep(void);

#ifdef __cplusplus
}
#endif

#endif