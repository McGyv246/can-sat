#ifndef I2C_WRAPPER_UTILS_H_
#define I2C_WRAPPER_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <types.h>

    sens_external_state_t write_reg_8(uint8_t addr, uint8_t reg, uint8_t value);

    sens_external_state_t read_reg_8(uint8_t addr, uint8_t reg, uint8_t *value);

    sens_external_state_t read_reg_16(uint8_t addr, uint8_t reg, uint16_t *value);

    sens_external_state_t read_reg_16_signed(uint8_t addr, uint8_t reg, int16_t *value);

    sens_external_state_t read_reg_buffered(uint8_t addr, uint8_t reg, uint8_t *buff, size_t len);

#ifdef __cplusplus
}
#endif

#endif