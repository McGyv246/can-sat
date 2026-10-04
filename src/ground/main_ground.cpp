#include <Arduino.h>
#include "types.h"
#include "debug_print.h"
#include "lora_drivers.h"

/*
Ground station firmware (env:ground in platformio.ini): receives the LoRa packets sent by the satellite.
Only core 0 is used (no setup1/loop1, so core 1 is never started).
*/

#define GROUND_SERIAL_WAIT_MS 5000

void setup()
{
  Serial.begin(115200);

  // USB serial drops what's printed before the monitor opens: wait a bit so the init messages are seen
  while (!Serial && millis() < GROUND_SERIAL_WAIT_MS)
    ;

  // SX1276 on SPI (CS D9, DIO0 D8, RESET D10), same wiring as the satellite. As receiver it starts listening right away
  if (lora_init(true) == SENS_STATE_READY)
    DBG_INFO("LORA receiver ready");
  else
    DBG_ERR("LORA init failed");
}

void loop()
{
  // TODO: read the packets with lora_read_packet, check the CRC and print/decode the data_to_lora_t
}
