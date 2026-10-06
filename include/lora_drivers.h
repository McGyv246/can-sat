#ifndef LORA_DRIVERS_H
#define LORA_DRIVERS_H

// callback function. should not be called from outside
void onTxDone();

// callback function. should not be called from outside
void onReceive();

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include "types.h"

    // initializes lora. if "is_receiver = true" lora is initialized as receiver and set to receive right away
    // otherwise, lora is set as sender and initialized in sleep mode
    sens_state_t lora_init(bool is_receiver);

    sens_state_t lora_wake(void);

    sens_state_t lora_send_packet(uint8_t *buffer, size_t len);

    // reads the last received packet into buffer (at most len bytes, the rest is discarded) and sets received_len.
    // returns SENS_STATE_READY if a packet was read, SENS_STATE_NOT_READY if none arrived, SENS_STATE_ERR if it was corrupted.
    // rssi, snr and frequency_error are optional. set to "nullptr" if unused
    sens_state_t lora_read_packet(uint8_t *buffer, size_t len, size_t *received_len, float *rssi, float *snr, float *frequency_error);

    void lora_sleep(void);

    // debug: raw IRQ flags register of the radio (LoRa mode: 0x40 RxDone, 0x20 PayloadCrcError,
    // 0x10 ValidHeader, 0x08 TxDone). Doesn't clear them
    uint16_t lora_get_irq_flags(void);

#ifdef __cplusplus
}
#endif

#endif