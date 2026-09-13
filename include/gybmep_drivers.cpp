#include<types.h>
#include<config.h>
#include<Arduino.h>
//IMPORTANTE: chiamare Wire.begin in setup (per il core I2C)
#include<Wire.h>

const uint32_t GYBMEP_WAIT_TIME_MS = ;
const uint8_t address = 0x76;
const uint8_t address_alternative = 0x77;
const uint8_t chip_id = 0x60;

static uint8_t current_address = address;
static sens_state_t state = SENS_IDLE;
static uint32_t last_sample_time;
static env_t latest_sample;

sens_status_t gybmep_init(void) {
    

}

sens_status_t gybmep_poll(uint32_t now_ms) {

    switch (state) {
        case SENS_IDLE:
            if (now_ms - last_sample_time < GYBMEP_WAIT_TIME_MS) return SENS_NOT_READY;

        case SENS_WAITING:

        case SENS_OK:
            return SENS_READY;
    }
    return SENS_ERR;

}

sens_status_t gybmep_get(env_t *data) {
    *data = latest_sample;
    return SENS_NOT_READY;
}

sens_status_t gybmep_sleep(void) {

}

