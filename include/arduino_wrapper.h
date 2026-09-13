#ifndef ARDUINO_WRAPPER_H
#define ARDUINO_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
    #include<Arduino.h>
    #define WRAPPER_INPUT INPUT
    #define WRAPPER_OUTPUT OUTPUT
    #define WRAPPER_INPUT_PULLUP INPUT_PULLUP
    #define WRAPPER_INPUT_PULLDOWN INPUT_PULLDOWN
    #define WRAPPER_LOW LOW
    #define WRAPPER_HIGH HIGH
#else
    #define WRAPPER_INPUT 0x0
    #define WRAPPER_OUTPUT 0x1
    #define WRAPPER_INPUT_PULLUP 0x2
    #define WRAPPER_INPUT_PULLDOWN 0x3
    #define WRAPPER_LOW 0x0
    #define WRAPPER_HIGH 0x1
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