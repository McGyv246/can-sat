#include<types.h>
#include<config.h>
#include<math.h>
#include<stdint.h>
#include<Arduino.h>

typedef enum {
    UV_IDLE,
    UV_SAMPLING,
    UV_VALIDATING,
    UV_READY,
} uv_state_t;

static uv_state_t state = UV_IDLE;
static uint16_t last_sample_time;
static light_t latest_sample;
//working voltage
const float VCC = 3.3;
const uint16_t MAX_V = 4095;
const float ADC_CONVERSION_FACTOR = VCC / (float)MAX_V;
//VOLTAGE_INDEX_FACTOR has to be verified
const float VOLTAGE_INDEX_FACTOR = 0.1;
//the correction factor alone could turn out to be not enough for accurate uv index measures.
//another correction factor could be needed to account for the offset voltage of the output
//const float VOLTAGE_OFFSET_FACTOR = ;
//this factor could be impossible to determine and vary across different operating conditions
//in case we decide to use it, it has to be subtracted to the measured voltage before dividing
//for VOLTAGE_INDEX_FACTOR
const float UV_TRESH = 2;
//to avoid incorrect samples due to noise a finite number of measures has to be
//sampled and then averaged
const int SAMPLE_NUMBER = 20;
const int SAMPLE_SPACING_MS = 1;
static uint16_t sample_sum;
static uint16_t sample_count;
static uint16_t sample_temp[SAMPLE_NUMBER];
static float sample_variance;
static uint16_t variance_count;
//standard deviation is yet to be estimated
const float MAX_DEV = 6.0;
static bool fast_sampling = false;

sens_status_t uv_init() {
    fast_sampling = true;
    return SENS_NOT_READY;
}

sens_status_t uv_poll(uint32_t now_ms) {

    switch (state) {
        case UV_IDLE:
            if (now_ms - last_sample_time < SAMPLE_SPACING_MS) return SENS_NOT_READY;
            sample_sum = 0;
            sample_count = 0;
            sample_variance = 0;
            variance_count = 0;
            sample_temp[sample_count] = analogRead(A_IN_UV);
            if(fast_sampling == true) {
                latest_sample.uv_raw = sample_temp[sample_count]; 
                latest_sample.uv_index = (float)(latest_sample.uv_raw * ADC_CONVERSION_FACTOR) / VOLTAGE_INDEX_FACTOR;
                if (latest_sample.uv_index > UV_TRESH) {
                    latest_sample.is_there_light = true;
                } else latest_sample.is_there_light = false;
                latest_sample.s.valid = true;
                state = UV_READY;
            } else {
                sample_sum += sample_temp[sample_count];
                sample_count ++;
                last_sample_time = now_ms;
                state = UV_SAMPLING;
            }


        case UV_SAMPLING:
            if (now_ms - last_sample_time < SAMPLE_SPACING_MS) return SENS_NOT_READY;
            sample_temp[sample_count] = analogRead(A_IN_UV);
            sample_sum += sample_temp[sample_count];
            sample_count ++;
            last_sample_time = now_ms;
            if (sample_count == SAMPLE_NUMBER) {
                latest_sample.uv_raw = (uint16_t)sample_sum/SAMPLE_NUMBER;
                float uv_voltage = (float)(latest_sample.uv_raw * ADC_CONVERSION_FACTOR) / VOLTAGE_INDEX_FACTOR;
                if (latest_sample.uv_index > UV_TRESH) {
                    latest_sample.is_there_light = true;
                } else latest_sample.is_there_light = false;
                state = UV_VALIDATING;
            }

        case UV_VALIDATING:
            last_sample_time = now_ms;
            do{
                sample_variance += pow((sample_temp[variance_count] - latest_sample.uv_raw), 2.0);
                variance_count ++;
                if (variance_count == SAMPLE_NUMBER) {
                    sample_variance /= (float)(SAMPLE_NUMBER - 1);
                    if (sqrt(sample_variance) < MAX_DEV) {
                        latest_sample.s.valid = true;
                    }else latest_sample.s.valid = false;
                    state = UV_READY;
                }
            } while(variance_count < SAMPLE_NUMBER);
            return SENS_NOT_READY;

        case UV_READY:
            latest_sample.s.t_ms = now_ms;
            last_sample_time = now_ms;
            state = UV_IDLE;
            return SENS_READY;
    }
    return SENS_ERR;

}

sens_status_t uv_get(light_t *data) {
    *data = latest_sample;
    return SENS_NOT_READY;
}

sens_status_t uv_sleep(void) {
    fast_sampling = false;
    return SENS_NOT_READY;
}
