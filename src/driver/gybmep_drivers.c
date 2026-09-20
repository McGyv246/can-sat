#include <Arduino.h>
#include "types.h"
#include "config.h"
#include "wire_wrapper.h"
#include "arduino_wrapper.h"
#include "I2C_wrapper_utils.h"

const uint8_t ADDRESS = 0x76;
const uint8_t ADDRESS_ALTERNATIVE = 0x77;
const uint8_t CHIP_ID = 0x60;
const float SEA_LEVEL_PRESSURE_HPA = 1013.25f; // check this value

// registers
static const uint8_t REG_RESET = 0xE0;
static const uint8_t REG_ID = 0xD0;
static const uint8_t REG_CTRL_HUM = 0xF2;
static const uint8_t REG_STATUS = 0xF3;
static const uint8_t REG_CTRL_MEAS = 0xF4;
static const uint8_t REG_CONFIG = 0xF5;
static const uint8_t REG_PRESS_MSB = 0xF7;

typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4, dig_H5;
    int8_t dig_H6;
} calibration_variables;

// settings, hardcoded for now. changing these settings from the outside shouldn't be needed
static const uint8_t OS_T = 2;       // temperature oversampling
static const uint8_t OS_P = 3;       // pressure oversampling
static const uint8_t OS_H = 1;       // humidity oversampling
static const uint8_t IIR_FILTER = 0; // built in filter to suppress disturbances. increases response time when active. 0 = off
static const uint8_t STANDBY = 3;    // standby setting for normal mode. can be set between 0.5ms and 1000ms. see datasheet
typedef enum
{
    MODE_SLEEP = 0,  // no operation, all registers accessible
    MODE_FORCED = 1, // samples once, stores it and then goes back to sleep
    MODE_NORMAL = 3, // continuous cycle of sampling and rest periods
} gybmep_mode_t;

// calculated using the formula for typical measurement time from the datasheet
static const uint32_t ESTIMATED_MEASURE_TIME_MS = (uint32_t)(1 + (2 * 2) + (2 * 4 + 0.5) + (2 * 1 + 0.5));

static uint8_t current_address = ADDRESS;
static sens_state_t state = SENS_IDLE;
static calibration_variables calib_data;
static env_t latest_sample;
static int32_t current_temp; // needed for pressure compensation

#pragma region internal_utils

static sens_status_t set_sampling(uint8_t os_t, uint8_t os_p, uint8_t os_h, uint8_t iir, uint8_t standby, uint8_t mode)
{
    os_t = os_t & 7;
    os_p = os_p & 7;
    os_h = os_h & 7;
    iir = iir & 7;
    standby = standby & 7;
    mode = mode & 3;

    if (write_reg_8(current_address, REG_CTRL_HUM, os_h & 0x07) == SENS_ERR)
        return SENS_ERR;

    uint8_t config = (uint8_t)((standby & 0x07) << 5 | (iir & 0x07) << 2);
    if (write_reg_8(current_address, REG_CONFIG, config) == SENS_ERR)
        return SENS_ERR;

    uint8_t ctrl_meas = (uint8_t)((os_t & 0x07) << 5 | (os_p & 0x07) << 2 | (mode & 0x03));
    if (write_reg_8(current_address, REG_CTRL_MEAS, ctrl_meas) == SENS_ERR)
        return SENS_ERR;

    return SENS_OK;
}

sens_status_t read_data_raw(int32_t *adc_P, int32_t *adc_T, int32_t *adc_H)
{
    size_t len = 8;
    uint8_t buf[len];
    if (read_reg_buffered(current_address, REG_PRESS_MSB, buf, len) == SENS_ERR)
        return SENS_ERR;
    *adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    *adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
    *adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];
    return SENS_OK;
}

float compensate_T(int32_t adc_T)
{
    int32_t temp_1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    int32_t temp_2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) *
                      ((int32_t)calib_data.dig_T3)) >>
                     14;
    current_temp = temp_1 + temp_2;
    int32_t temp_C = (current_temp * 5 + 128) >> 8;
    return (float)temp_C / 100.0f; // C°
}

float compensate_P(int32_t adc_P)
{
    int64_t temp_1 = ((int64_t)current_temp) - 128000;
    int64_t temp_2 = temp_1 * temp_1 * (int64_t)calib_data.dig_P6;
    temp_2 = temp_2 + ((temp_1 * (int64_t)calib_data.dig_P5) << 17);
    temp_2 = temp_2 + (((int64_t)calib_data.dig_P4) << 35);
    temp_1 = ((temp_1 * temp_1 * (int64_t)calib_data.dig_P3) >> 8) + ((temp_1 * (int64_t)calib_data.dig_P2) << 12);
    temp_1 = (((((int64_t)1) << 47) + temp_1) * (int64_t)calib_data.dig_P1) >> 33;
    if (temp_1 == 0)
        return 0.0f;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - temp_2) * 3125) / temp_1;
    temp_1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    temp_2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
    p = ((p + temp_1 + temp_2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
    return (float)p / 256.0f; // Pa
}

float compensate_H(int32_t adc_H)
{
    float temp_H = (float)current_temp - 76800.0f;
    temp_H = ((float)adc_H - ((float)calib_data.dig_H4 * 64.0f + ((float)calib_data.dig_H5 / 16384.0f) * temp_H)) * (((float)calib_data.dig_H2 / 65536.0f) * (1.0f + ((float)calib_data.dig_H6 / 67108864.0f) * temp_H * (1.0f + ((float)calib_data.dig_H3 / 67108864.0f) * temp_H)));
    temp_H = temp_H * (1.0f - ((float)calib_data.dig_H1 * temp_H / 524288.0f));
    if (temp_H > 100.0f)
        temp_H = 100.0f;
    if (temp_H < 0.0f)
        temp_H = 0.0f;
    return temp_H; // %Relative Humidity
}

float calculate_altitude(float temperature, float pressure)
{
    return 44330.0f * (1.0f - powf(pressure / SEA_LEVEL_PRESSURE_HPA, 0.1903f));
}

#pragma endregion

#pragma region setup

static sens_status_t read_calibration(void)
{
    if (read_reg_16(current_address, 0x88, &calib_data.dig_T1) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x8A, &calib_data.dig_T2) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x8C, &calib_data.dig_T3) == SENS_ERR)
        return SENS_ERR;

    if (read_reg_16(current_address, 0x8E, &calib_data.dig_P1) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x90, &calib_data.dig_P2) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x92, &calib_data.dig_P3) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x94, &calib_data.dig_P4) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x96, &calib_data.dig_P5) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x98, &calib_data.dig_P6) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x9A, &calib_data.dig_P7) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x9C, &calib_data.dig_P8) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0x9E, &calib_data.dig_P9) == SENS_ERR)
        return SENS_ERR;

    if (read_reg_8(current_address, 0xA1, &calib_data.dig_H1) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_16_signed(current_address, 0xE1, &calib_data.dig_H2) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_8(current_address, 0xE3, &calib_data.dig_H3) == SENS_ERR)
        return SENS_ERR;

    uint8_t lo;
    uint8_t mid;
    uint8_t hi;

    if (read_reg_8(current_address, 0xE4, &lo) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_8(current_address, 0xE5, &mid) == SENS_ERR)
        return SENS_ERR;
    if (read_reg_8(current_address, 0xE6, &hi) == SENS_ERR)
        return SENS_ERR;

    calib_data.dig_H4 = (int16_t)((((int16_t)lo) << 4) | (mid & 0x0F));
    if (calib_data.dig_H4 & 0x0800)
        calib_data.dig_H4 -= 4096;

    calib_data.dig_H5 = (int16_t)((((int16_t)hi) << 4) | (mid >> 4));
    if (calib_data.dig_H5 & 0x0800)
        calib_data.dig_H5 -= 4096;

    uint8_t temp_H6;
    if (read_reg_8(current_address, 0xE7, &temp_H6) == SENS_ERR)
        return SENS_ERR;
    calib_data.dig_H6 = (int8_t)temp_H6;
    return SENS_OK;
}

// to be called once on setup
sens_status_t gybmep_init(void)
{
    uint8_t id;
    if (read_reg_8(ADDRESS, REG_ID, &id) != SENS_OK)
    {
        if (read_reg_8(ADDRESS_ALTERNATIVE, REG_ID, &id) != SENS_OK)
        {
            return SENS_ERR; // no address is correct (device might be disconnected)
        }
        current_address = ADDRESS_ALTERNATIVE;
    }
    current_address = ADDRESS;
    if (id != CHIP_ID)
        return SENS_ERR;
    write_reg_8(current_address, REG_RESET, 0xB6); // resets device
    delay_ms((uint32_t)3);
    while (true)
    {
        uint8_t status;
        if (read_reg_8(current_address, REG_STATUS, &status) == SENS_ERR)
            return SENS_ERR; // reads im_update bit
        if (!(status & 0x01))
            break;   // checks for im_update bit to be zero
        delay_ms(1); // waits for reset to be completed
    }
    if (read_calibration() == SENS_ERR)
        return SENS_ERR;
    if (set_sampling(OS_T, OS_P, OS_H, IIR_FILTER, STANDBY, MODE_SLEEP) == SENS_ERR)
        return SENS_ERR;
    return SENS_NOT_READY;
}

#pragma endregion

#pragma region public_interface_functions

sens_status_t gybmep_wake(void)
{
    if (set_sampling(OS_T, OS_P, OS_H, IIR_FILTER, STANDBY, MODE_NORMAL) == SENS_ERR)
        return SENS_ERR;
    state = SENS_IDLE;
    return SENS_NOT_READY;
}

sens_status_t gybmep_poll(uint32_t now_ms)
{
    switch (state)
    {
    case SENS_IDLE:
        uint8_t status;
        if (read_reg_8(current_address, REG_STATUS, &status) == SENS_ERR)
            return SENS_ERR;
        if (status & 0x08)
            return SENS_NOT_READY; // checks measuring bit to see if a measure is in progress
        int32_t adc_P, adc_T, adc_H;
        if (read_data_raw(&adc_P, &adc_T, &adc_H) == SENS_ERR)
            return SENS_ERR;
        latest_sample.temp_c = compensate_T(adc_T); // compensate_T MUST be the first compensation called
        latest_sample.pressure_pa = compensate_P(adc_P);
        latest_sample.humidity_pct = compensate_H(adc_H);
        latest_sample.altitude_m = calculate_altitude(latest_sample.temp_c, latest_sample.pressure_pa);
        latest_sample.s.t_ms = now_ms - ESTIMATED_MEASURE_TIME_MS;
        latest_sample.s.valid = true;
        state = SENS_OK;

    case SENS_WAITING:
        // no conversion needed
        return SENS_ERR;

    case SENS_OK:
        return SENS_READY;
    }
    return SENS_ERR;
}

sens_status_t gybmep_get(env_t *data)
{
    *data = latest_sample;
    return SENS_NOT_READY;
}

sens_status_t gybmep_sleep(void)
{
    if (set_sampling(OS_T, OS_P, OS_H, IIR_FILTER, STANDBY, MODE_SLEEP) == SENS_ERR)
        return SENS_ERR;
    state = SENS_IDLE;
    return SENS_NOT_READY;
}

#pragma endregion