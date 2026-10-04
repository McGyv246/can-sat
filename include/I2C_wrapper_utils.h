#ifndef I2C_WRAPPER_UTILS_H
#define I2C_WRAPPER_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <types.h>

    sens_state_t write_reg_8(uint8_t addr, uint8_t reg, uint8_t value);

    sens_state_t read_reg_8(uint8_t addr, uint8_t reg, uint8_t *value);

    sens_state_t read_reg_16(uint8_t addr, uint8_t reg, uint16_t *value);

    sens_state_t read_reg_16_signed(uint8_t addr, uint8_t reg, int16_t *value);

    sens_state_t read_reg_buffered(uint8_t addr, uint8_t reg, uint8_t *buff, size_t len);

    sens_state_t read_reg_16_big_endian(uint8_t addr, uint16_t reg, uint16_t *value);

#ifdef __cplusplus
}
#endif

#endif