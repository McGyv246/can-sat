#ifndef ARDUINO_WRAPPER_H
#define ARDUINO_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
#include <Arduino.h>
#define WRAPPER_INPUT INPUT
#define WRAPPER_OUTPUT OUTPUT
#define WRAPPER_INPUT_PULLUP INPUT_PULLUP
#define WRAPPER_INPUT_PULLDOWN INPUT_PULLDOWN
#define WRAPPER_LOW LOW
#define WRAPPER_HIGH HIGH
#define WRAPPER_D0 D0
#define WRAPPER_D1 D1
#define WRAPPER_D2 D2
#define WRAPPER_D3 D3
#define WRAPPER_D4 D4
#define WRAPPER_D5 D5
#define WRAPPER_D6 D6
#define WRAPPER_D7 D7
#define WRAPPER_D8 D8
#define WRAPPER_D9 D9
#define WRAPPER_D10 D10
#define WRAPPER_D11 D11
#define WRAPPER_D12 D12
#define WRAPPER_D13 D13
#define WRAPPER_D14 D14
#define WRAPPER_D15 D15
#define WRAPPER_D16 D16
#define WRAPPER_D17 D17
#define WRAPPER_D18 D18
#define WRAPPER_D19 D19
#define WRAPPER_D20 D20
#define WRAPPER_D21 D21
#else
#define WRAPPER_INPUT 0x0
#define WRAPPER_OUTPUT 0x1
#define WRAPPER_INPUT_PULLUP 0x2
#define WRAPPER_INPUT_PULLDOWN 0x3
#define WRAPPER_LOW 0x0
#define WRAPPER_HIGH 0x1
#endif

    void wrapper_pin_mode(uint8_t pin, uint8_t mode);
    void wrapper_digital_write(uint8_t pin, uint8_t val);
    int wrapper_digital_read(uint8_t pin);
    int wrapper_analog_read(uint8_t pin);
    void wrapper_delay_ms(uint32_t ms);
    uint32_t wrapper_millis(void);

#ifdef __cplusplus
}
#endif

#endif