#include <Arduino.h>
#include "config.h"

void setup()
{
}

void loop()
{
  // dummy loop
  Serial.print("PHOTORESISTOR PIN: ");
  Serial.print(D_IN_PHOTORESISTOR);
  Serial.print("\n");
  delay(2000);
}

void setup1()
{
}

void loop1()
{
  // dummy loop1
  Serial.print("BLUE PIN: ");
  Serial.print(PWM_OUT_BLUE);
  Serial.print("\n");
  delay(3000);
}