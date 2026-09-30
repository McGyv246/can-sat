#include <stdint.h>
#include <stddef.h>
#include <LoRa.h>
#include "types.h"
#include "lora_drivers.h"
#include "config_arduino.h"
#include "arduino_wrapper.h"

// uncomment the chosen lora setting

// 868 MHz duty cycle and tx power optimization (this would use meshcore and meshtastic frequencies. not sure if that could cause problems)
const long TX_FREQUENCY = 869525E3; // temp
const int TX_POWER = 20;            // TX power in dB.
const int SPREADING_FACTOR = 7;     // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
const long SIGNAL_BANDWIDTH = 125E3;
const volatile float PERMITTED_DUTY_CYCLE = 0.1f;

// 868 MHz duty cycle and air time optimization (increase spreading factor of 1 or 2 to increase signal strength and maintain decent speed)
// i'm not sure if increasing tx power would be into legal limits
/*
const long TX_FREQUENCY = 8683E5; // temp
const int TX_POWER = 14;         // TX power in dB.
const int SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
const long SIGNAL_BANDWIDTH = 250E3;
const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
*/

// 868 MHz illegal ass signal strength + air time + duty cycle optimization
/*
const long TX_FREQUENCY = 8683E5; // temp
const int TX_POWER = 20;         // TX power in dB.
const int SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
const long SIGNAL_BANDWIDTH = 250E3;
const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
*/

// 433 MHz air time optimization
/*
const long TX_FREQUENCY = 43342E4; // temp
const int TX_POWER = 10;         // TX power in dB.
const int SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
const long SIGNAL_BANDWIDTH = 250E3;
const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
*/

// 433 MHz signal strength optimization (this could be sloooooooooooooow)
/*
const long TX_FREQUENCY = 43342E4; // temp
const int TX_POWER = 10;         // TX power in dB.
const int SPREADING_FACTOR = 8;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
const long SIGNAL_BANDWIDTH = 62.5E3;
const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
*/

// 433 MHz illegal ass signal strength + air time optimization
/*
const long TX_FREQUENCY = 43342E4; // temp
const int TX_POWER = 20;         // TX power in dB.
const int SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
const long SIGNAL_BANDWIDTH = 250E3;
const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
*/

const int CODING_RATE_DENOMINATOR = 5; /* must be between 5 and 8. increasing this could be useful in case of frequent package corruption due to interference,
                                        but it will increase transmission time */
const int SYNC_WORD_LORA = 0x12;       // default
const bool ASYNC_MODE = true;          // true enables non-blocking mode

// sender globals
static volatile bool busy = false;
static volatile uint32_t tx_start_time;
static volatile uint32_t tx_end_time;
static volatile uint32_t wait_time;

// receiver globals
static bool receiver = false;
static volatile bool packet_received = false;
static volatile size_t received_size = 0;

void onTxDone()
{
    tx_end_time = wrapper_millis();
    busy = false;
    wait_time = (uint32_t)(((float)(tx_end_time - tx_start_time) / PERMITTED_DUTY_CYCLE) * (1.0f - PERMITTED_DUTY_CYCLE));
}

void onReceive(int packet_size)
{
    packet_received = true;
    received_size = (size_t)packet_size;
    return;
}

sens_state_t lora_init(bool is_receiver)
{
    receiver = is_receiver;
    LoRa.setPins(CHIPSELECT_LORA, RESET_LORA, DIO_LORA);
    if (!LoRa.begin(TX_FREQUENCY))
        return SENS_STATE_ERR;
    LoRa.setTxPower(TX_POWER);
    LoRa.setSpreadingFactor(SPREADING_FACTOR);
    LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH);
    LoRa.setCodingRate4(CODING_RATE_DENOMINATOR);
    LoRa.setSyncWord(SYNC_WORD_LORA);
    if (receiver)
    {
        LoRa.onReceive(onReceive);
        LoRa.receive();
        return SENS_STATE_READY;
    }
    LoRa.onTxDone(onTxDone);
    LoRa.sleep();
    return SENS_STATE_READY;
}

void lora_wake(void)
{
    LoRa.idle();
    if (receiver)
        LoRa.receive();
    return;
}

sens_state_t lora_send_packet(uint8_t *buffer, size_t len)
{
    if (receiver)
        return SENS_STATE_ERR;
    if ((wrapper_millis() - tx_end_time) < wait_time)
        return SENS_STATE_NOT_READY;
    if (busy)
        return SENS_STATE_NOT_READY;
    if (!LoRa.beginPacket())
        return SENS_STATE_ERR;
    LoRa.write(buffer, len);
    if (!LoRa.endPacket(ASYNC_MODE))
        return SENS_STATE_ERR;
    tx_start_time = wrapper_millis();
    busy = true;
    return SENS_STATE_READY;
}

sens_state_t lora_read_packet(uint8_t *buffer, size_t len)
{
    if (!packet_received)
        return SENS_STATE_NOT_READY;
    if (len < received_size)
        return SENS_STATE_ERR;
    LoRa.readBytes(buffer, received_size);
    packet_received = false;
    return SENS_STATE_NOT_READY;
}

void lora_sleep(void)
{
    LoRa.sleep();
    return;
}