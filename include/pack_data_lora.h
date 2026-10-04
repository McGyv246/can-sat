#ifndef PACK_DATA_LORA_H
#define PACK_DATA_LORA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include "types.h"

#define LORA_VERSION 1 /* increase it whenever the packet layout changes */

/* Bits of the "status" field: 1 = that block's last read succeeded. */
#define STATUS_ENV_OK (1u << 0)
#define STATUS_IMU_OK (1u << 1)
#define STATUS_GPS_OK (1u << 2)
#define STATUS_UV_OK (1u << 3)
#define STATUS_FOTORES_OK (1u << 4)
#define STATUS_GAS_OK (1u << 5)
#define STATUS_BATTERY_OK (1u << 6)
#define STATUS_BATTERY_ALERT (1u << 7)
#define STATUS_HEATER_ON (1u << 8)

    void ds_to_lora(data_to_lora_t *out, datastore_t *data);

    /*
    Checks a received packet (length, CRC, sync word, version, in this order) and, if it's ok,
    converts it back to physical units in out. out is left untouched if the packet is rejected
    */
    lora_decode_result_t lora_decode(lora_decoded_t *out, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
