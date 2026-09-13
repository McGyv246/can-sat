#include <Arduino.h>
#include "arduino_wrapper.h"

extern "C" {

void ard_pin_mode(uint8_t pin, uint8_t mode) {
  pinMode(pin, mode);
}

void ard_digital_write(uint8_t pin, uint8_t val) {
  digitalWrite(pin, val);
}

int ard_digital_read(uint8_t pin) {
  return digitalRead(pin);
}

int ard_analog_read(uint8_t pin) {
  return analogRead(pin);
}

void ard_delay_ms(uint32_t ms) {
  delay(ms);
}

uint32_t ard_millis(void) {
  return millis();
}

}