#include "types.h"
#include "wire_wrapper.h"
#include <stddef.h>

sens_state_t write_reg_8(uint8_t addr, uint8_t reg, uint8_t value)
{
    wire_begin_transmission(addr);
    wire_write(reg);
    wire_write(value);
    if (wire_end_transmission(true) != 0)
        return SENS_STATE_ERR; // transmission was unsuccessful. device might be disconnected
    // return value of "wire_end_transmission" could be analyzed to determine point of failure
    return SENS_STATE_READY;
}

sens_state_t read_reg_8(uint8_t addr, uint8_t reg, uint8_t *value)
{
    wire_begin_transmission(addr);
    wire_write(reg);
    if (wire_end_transmission(false) != 0)
    {
        return SENS_STATE_ERR; // no ack was received (device might be disconnected)
    }
    if (wire_request_from(addr, (uint8_t)1, true) != 1)
        return SENS_STATE_ERR; // no bytes were returned from the device
    if (!wire_available())
        return SENS_STATE_ERR; // no bytes are available for reading
    *value = wire_read();
    return SENS_STATE_READY;
}

// only use on little endian registers!!!!!
sens_state_t read_reg_16(uint8_t addr, uint8_t reg, uint16_t *value)
{
    wire_begin_transmission(addr);
    wire_write(reg);
    if (wire_end_transmission(false) != 0)
    {
        return SENS_STATE_ERR; // no ack was received (device might be disconnected)
    }
    if (wire_request_from(addr, (uint8_t)2, true) != 2)
        return SENS_STATE_ERR; // no bytes (or not all of them) were returned from the device
    if (!wire_available())
        return SENS_STATE_ERR; // no bytes are available for reading
    uint8_t lo = wire_read();
    if (!wire_available())
        return SENS_STATE_ERR; // no bytes are available for reading
    uint8_t hi = wire_read();
    *value = ((uint16_t)hi << 8) | lo;
    return SENS_STATE_READY;
}

sens_state_t read_reg_16_signed(uint8_t addr, uint8_t reg, int16_t *value)
{
    uint16_t temp;
    if (read_reg_16(addr, reg, &temp) == SENS_STATE_ERR)
        return SENS_STATE_ERR;
    *value = (int16_t)temp;
    return SENS_STATE_READY;
}

sens_state_t read_reg_buffered(uint8_t addr, uint8_t reg, uint8_t *buff, size_t len)
{
    wire_begin_transmission(addr);
    wire_write(reg);
    if (wire_end_transmission(false) != 0)
    {
        return SENS_STATE_ERR; // no ack was received (device might be disconnected)
    }
    if (wire_request_from(addr, len, true) != len)
        return SENS_STATE_ERR; // no bytes (or not all of them) were returned from the device
    if (!wire_available())
        return SENS_STATE_ERR; // no bytes are available for reading
    for (size_t i = 0; i < len && wire_available(); i++)
        buff[i] = wire_read();
    return SENS_STATE_READY;
}