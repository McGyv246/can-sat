#ifndef CRC_H
#define CRC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

    uint16_t crc16(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif