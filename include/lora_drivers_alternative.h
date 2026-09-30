#ifndef LORA_DRIVERS_ALTERNATIVE_H_
#define LORA_DRIVERS_ALTERNATIVE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include "types.h"

    // callback function. should not be called from outside
    void onTxDone();

    // callback function. should not be called from outside
    void onReceive();

    // initializes lora. if "is_receiver = true" lora is initialized as receiver and set to receive right away
    // otherwise, lora is set as sender and initialized in sleep mode
    sens_state_t lora_init(bool is_receiver);

    sens_state_t lora_wake(void);

    sens_state_t lora_send_packet(uint8_t *buffer, size_t len);

    // rssi, snr and frequency_error are optional. set to "nullptr" if unused
    sens_state_t lora_read_packet(uint8_t *buffer, size_t len, float *rssi, float *snr, float *frequency_error);

    void lora_sleep(void);

#ifdef __cplusplus
}
#endif

#endif