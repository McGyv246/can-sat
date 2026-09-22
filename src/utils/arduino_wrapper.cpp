#include <Arduino.h>

extern "C"
{

  void pin_mode(uint8_t pin, uint8_t mode)
  {
    pinMode(pin, mode);
  }

  void digital_write(uint8_t pin, uint8_t val)
  {
    digitalWrite(pin, val);
  }

  int digital_read(uint8_t pin)
  {
    return digitalRead(pin);
  }

  int analog_read(uint8_t pin)
  {
    return analogRead(pin);
  }

  void delay_ms(uint32_t ms)
  {
    delay(ms);
  }

  uint32_t millis(void)
  {
    return (uint32_t)millis();
  }
}