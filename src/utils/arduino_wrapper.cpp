#include <Arduino.h>

extern "C"
{

  void wrapper_pin_mode(uint8_t pin, uint8_t mode)
  {
    pinMode(pin, mode);
  }

  void wrapper_digital_write(uint8_t pin, uint8_t val)
  {
    digitalWrite(pin, val);
  }

  int wrapper_digital_read(uint8_t pin)
  {
    return digitalRead(pin);
  }

  void wrapper_analog_write(uint8_t pin, int val)
  {
    return analogWrite(pin, val);
  }

  int wrapper_analog_read(uint8_t pin)
  {
    return analogRead(pin);
  }

  void wrapper_delay_ms(uint32_t ms)
  {
    delay(ms);
  }

  uint32_t wrapper_millis(void)
  {
    return (uint32_t)millis();
  }
}