#ifndef ARDUINO_WRAPPER_H
#define ARDUINO_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     pin_mode(uint8_t pin, uint8_t mode);
void     digital_write(uint8_t pin, uint8_t val);
int      digital_read(uint8_t pin);
int      analog_read(uint8_t pin);
void     delay_ms(uint32_t ms);
uint32_t millis(void);

#ifdef __cplusplus
}
#endif

#endif