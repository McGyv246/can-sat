#ifndef WIRE_WRAPPER_H
#define WIRE_WRAPPER_H

#include <Arduino.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void wire_begin_service(void);

    void wire_begin_transmission(uint8_t address);

    size_t wire_write(uint8_t send);

    uint8_t wire_end_transmission(bool stop);

    uint8_t wire_request_from(uint8_t address, size_t quantity, bool stop);

    int wire_available(void);

    int wire_read(void);

#ifdef __cplusplus
}
#endif

#endif