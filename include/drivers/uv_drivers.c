#include<types.h>
#include<config.h>
#include<math.h>
#include<stdint.h>
#include<Arduino.h>

typedef enum {
    UV_IDLE,
    UV_SAMPLING,
    UV_VALIDATING,
    UV_SLOW_SAMPLE,
    UV_READY,
} uv_state_t;

static uv_state_t state = UV_IDLE;
static uint16_t last_sample_time;
static light_t latest_sample;
//working voltage
const float VCC = 3.3;
const uint16_t MAX_V = 4095;
//da verificare
const float VOLTAGE_INDEX_FACTOR = 0.1;
const float UV_TRESH = 1;
//to avoid incorrect samples due to noise a finite number of measures has to be
//sampled and then averaged
const int SAMPLE_NUMBER = 20;
const int SAMPLE_SPACING_MS = 2;
static uint16_t sample_sum;
static uint16_t sample_count;
static uint16_t sample_temp[SAMPLE_NUMBER];
static float sample_variance;
static uint16_t variance_count;
//la deviazione standard massima va determinata sperimentalmente
const float MAX_DEV = 6.0;

sens_status_t uv_poll(uint32_t now_ms) {

    switch (state) {
        case UV_IDLE:
            if (now_ms - last_sample_time < SAMPLE_SPACING_MS) return SENS_NOT_READY;
            //hal adc read non esiste ancora, non so se prende un pin come parametro
            sample_sum = 0;
            sample_count = 0;
            sample_variance = 0;
            variance_count = 0;
            sample_temp[sample_count] = analogRead(A_IN_UV);
            sample_count ++;
            sample_sum += sample_temp[sample_count - 1];
            last_sample_time = now_ms;
            state = UV_SAMPLING;

        case UV_SAMPLING:
            if (now_ms - last_sample_time < SAMPLE_SPACING_MS) return SENS_NOT_READY;
            sample_temp[sample_count] = analogRead(A_IN_UV);
            sample_count ++;
            sample_sum += sample_temp[sample_count - 1];
            last_sample_time = now_ms;
            if (sample_count == SAMPLE_NUMBER) {
                latest_sample.uv_raw = (uint16_t)sample_sum/SAMPLE_NUMBER;
                int uv_voltage = (int)latest_sample.uv_raw * (int)(VCC/(float)MAX_V);
                latest_sample.uv_index = uv_voltage/VOLTAGE_INDEX_FACTOR;
                if (latest_sample.uv_index > UV_TRESH) {
                    latest_sample.is_there_light = true;
                } else latest_sample.is_there_light = false;
                state = UV_VALIDATING;
            }

        case UV_VALIDATING:
            sample_variance += pow((sample_temp[variance_count] - sample_sum), 2.0);
            variance_count ++;
            last_sample_time = now_ms;
            if (variance_count == SAMPLE_NUMBER) {
                sample_variance /= (float)(SAMPLE_NUMBER - 1);
                if (sqrt(sample_variance) < MAX_DEV) {
                    latest_sample.s.valid = true;
                }else latest_sample.s.valid = false;
                state = UV_READY;
            }
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
    //capire se queste righe possono avere un senso oppure se può tornare void e basta
    if(latest_sample.s.valid) return SENS_READY;
    return SENS_NOT_READY;
}
