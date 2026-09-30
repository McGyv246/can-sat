#include <Wire.h>
#include <stddef.h>
#include <Arduino.h> //serve per avere il tipo "byte"
#include "wire_wrapper.h"

extern "C"
{
    void wire_begin_service(void)
    {
        Wire.begin();
    }

    void wire_begin_transmission(uint8_t address)
    {
        Wire.beginTransmission(address);
    }

    size_t wire_write(uint8_t send)
    {
        return Wire.write(send);
    }

    uint8_t wire_end_transmission(bool stop)
    {
        return Wire.endTransmission(stop);
    }

    uint8_t wire_request_from(uint8_t address, size_t quantity, bool stop)
    {
        return Wire.requestFrom(address, quantity, stop);
    }

    int wire_available(void)
    {
        return Wire.available();
    }

    int wire_read(void)
    {
        return Wire.read();
    }
}