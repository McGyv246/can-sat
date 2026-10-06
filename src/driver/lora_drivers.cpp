#include <stdint.h>
#include <stddef.h>
#include <RadioLib.h>
#include "types.h"
#include "config_arduino.h"
#include "arduino_wrapper.h"

// uncomment the chosen lora setting

// 868 MHz duty cycle and tx power optimization (this would use meshcore and meshtastic frequencies. not sure if that could cause problems)
static const float TX_FREQUENCY = 869.525f;
static const int TX_POWER = 20;            // TX power in dB.
static const uint8_t SPREADING_FACTOR = 7; // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
static const float SIGNAL_BANDWIDTH = 125.0f;
static const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
static const bool IGNORE_DUTY_CYCLE = false;

// 868 MHz duty cycle and air time optimization (increase spreading factor of 1 or 2 to increase signal strength and maintain decent speed)
// i'm not sure if increasing tx power would be into legal limits
/*
static const float TX_FREQUENCY = 868.3;
static const int TX_POWER = 14;         // TX power in dB.
static const uint8_t SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
static const float SIGNAL_BANDWIDTH = 250.0f;
static const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
static const bool IGNORE_DUTY_CYCLE = false;
*/

// 868 MHz illegal ass signal strength + air time + duty cycle optimization. the way mike fenice wanted it to be. MAXIMUM POWER
/*
static const float TX_FREQUENCY = 868.3;
static const int TX_POWER = 20;         // TX power in dB.
static const uint8_t SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
static const float SIGNAL_BANDWIDTH = 250.0f;
static const volatile float PERMITTED_DUTY_CYCLE = 1.0f;
static const bool IGNORE_DUTY_CYCLE = true;
*/

// 433 MHz air time optimization
/*
static const float TX_FREQUENCY = 433.42;
static const int TX_POWER = 10;         // TX power in dB.
static const uint8_t SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
static const float SIGNAL_BANDWIDTH = 250.0f;
static const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
static const bool IGNORE_DUTY_CYCLE = false;
*/

// 433 MHz signal strength optimization (this could be sloooooooooooooow)
/*
static const float TX_FREQUENCY = 433.42;
static const int TX_POWER = 10;         // TX power in dB.
static const uint8_t SPREADING_FACTOR = 8;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
static const float SIGNAL_BANDWIDTH = 62.5f;
static const volatile float PERMITTED_DUTY_CYCLE = 0.1f;
static const bool IGNORE_DUTY_CYCLE = false;
*/

// 433 MHz illegal ass signal strength + air time + duty cycle optimization. MAXIMUM POWER
/*
static const float TX_FREQUENCY = 433.42;
static const int TX_POWER = 20;         // TX power in dB.
static const uint8_t SPREADING_FACTOR = 7;  // must be between 6 and 12. if 6 is used, implicit header mode must be used to transmit and receive packages (DON'T USE 6)
static const float SIGNAL_BANDWIDTH = 250.0f;
static const volatile float PERMITTED_DUTY_CYCLE = 1.0f;
static const bool IGNORE_DUTY_CYCLE = true;
*/

static const int CODING_RATE_DENOMINATOR = 5; /* must be between 5 and 8. increasing this could be useful in case of frequent package corruption due to interference,
                                        but it will increase transmission time */
static const int SYNC_WORD_LORA = 0x12;       // default

// sender globals
static volatile bool busy = false;
static volatile uint32_t tx_start_time;
static volatile uint32_t tx_end_time;
static volatile uint32_t wait_time = 0; // no wait before the first packet, onTxDone sets it after each one
static bool flushed = true;

// receiver globals
static bool receiver = false;
static volatile bool packet_received = false;

static SX1276 lora = new Module(CHIPSELECT_LORA, DIO_LORA, RESET_LORA);
static ConfigLoRa_t config;

void onTxDone()
{
    busy = false;
    if (!IGNORE_DUTY_CYCLE)
    {
        tx_end_time = wrapper_millis();
        wait_time = (uint32_t)(((float)(tx_end_time - tx_start_time) / PERMITTED_DUTY_CYCLE) * (1.0f - PERMITTED_DUTY_CYCLE));
    }
}

void onReceive()
{
    packet_received = true;
    return;
}

extern "C"
{
    sens_state_t lora_init(bool is_receiver)
    {
        receiver = is_receiver;
        config.frequency = TX_FREQUENCY;
        config.bandwidth = SIGNAL_BANDWIDTH;
        config.spreadingFactor = SPREADING_FACTOR;
        config.codingRate = CODING_RATE_DENOMINATOR;
        config.syncWord = SYNC_WORD_LORA;
        config.power = TX_POWER;
        lora.gain = 0; // automatic gain control
        if (lora.begin(config) != RADIOLIB_ERR_NONE)
            return SENS_STATE_ERR;
        if (receiver)
        {
            lora.setPacketReceivedAction(onReceive);
            if (lora.startReceive() != RADIOLIB_ERR_NONE)
                return SENS_STATE_ERR;
            return SENS_STATE_READY;
        }
        lora.setPacketSentAction(onTxDone);
        lora.sleep();
        return SENS_STATE_READY;
    }

    sens_state_t lora_wake(void)
    {
        lora.standby();
        if (receiver)
        {
            if (lora.startReceive() != RADIOLIB_ERR_NONE)
                return SENS_STATE_ERR;
        }
        return SENS_STATE_NOT_READY;
    }

    sens_state_t lora_send_packet(uint8_t *buffer, size_t len)
    {
        if (receiver)
            return SENS_STATE_ERR;
        if (((wrapper_millis() - tx_end_time) < wait_time) && !IGNORE_DUTY_CYCLE)
            return SENS_STATE_NOT_READY;
        if (busy)
            return SENS_STATE_NOT_READY;
        if (!flushed)
        {
            if (lora.finishTransmit() != RADIOLIB_ERR_NONE)
                return SENS_STATE_ERR;
            flushed = true;
        }
        if (lora.startTransmit(buffer, len) != RADIOLIB_ERR_NONE)
            return SENS_STATE_ERR;
        tx_start_time = wrapper_millis();
        busy = true;
        flushed = false;
        return SENS_STATE_READY;
    }

    sens_state_t lora_read_packet(uint8_t *buffer, size_t len, size_t *received_len, float *rssi, float *snr, float *frequency_error)
    {
        *received_len = 0;
        if (!receiver)
            return SENS_STATE_ERR;
#ifdef LORA_POLL_RX
        // no DIO0 wire: the flag register is the only source, a floating pin could fire onReceive with no packet.
        // readData clears the flags and the radio stays in continuous receive
        packet_received = (lora.getIRQFlags() & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE) != 0;
#endif
        if (!packet_received)
            return SENS_STATE_NOT_READY;
        // cleared before reading: if a new packet arrives meanwhile the ISR sets it again and it's not lost
        packet_received = false;

        // the length must be read before readData, which clears it. Bytes past len are discarded by readData
        size_t packet_len = lora.getPacketLength();
        if (lora.readData(buffer, len) != RADIOLIB_ERR_NONE) // e.g. the radio's own CRC failed
            return SENS_STATE_ERR;
        *received_len = packet_len < len ? packet_len : len;

        if (rssi != nullptr)
            *rssi = lora.getRSSI();
        if (snr != nullptr)
            *snr = lora.getSNR();
        if (frequency_error != nullptr)
            *frequency_error = lora.getFrequencyError();
        return SENS_STATE_READY;
    }

    void lora_sleep(void)
    {
        lora.sleep();
        return;
    }

    uint16_t lora_get_irq_flags(void)
    {
        return lora.getIRQFlags();
    }
}