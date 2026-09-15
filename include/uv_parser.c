#include <math.h>
#include "types.h"

// working voltage
const float VCC = 3.3;
const uint16_t MAX_V = 4095;
const float ADC_CONVERSION_FACTOR = VCC / (float)MAX_V;
// VOLTAGE_INDEX_FACTOR has to be verified
const float VOLTAGE_INDEX_FACTOR = 0.1;
// the correction factor alone could turn out to be not enough for accurate uv index measures.
// another correction factor could be needed to account for the offset voltage of the output
// const float VOLTAGE_OFFSET_FACTOR = ;
// this factor could be impossible to determine and vary across different operating conditions
// in case we decide to use it, it has to be subtracted to the measured voltage before dividing
// for VOLTAGE_INDEX_FACTOR
const float UV_TRESH = 2;
// to avoid incorrect samples due to noise a finite number of measures has to be
// sampled and then averaged
// standard deviation is yet to be estimated
const float MAX_DEV = 6.0;

light_t uv_parse(light_t data[])
{
    light_t parsed_sample;
    size_t sample_count = sizeof(data) / sizeof(data[0]);
    uint32_t mean_time = 0;
    uint16_t uv_raw_mean = 0;
    for (size_t i = 0; i < sample_count; i++)
    {
        mean_time += data[i].s.t_ms;
        uv_raw_mean += data[i].uv_raw;
    }
    mean_time /= sample_count;
    parsed_sample.s.t_ms = mean_time; // modo temporaneo di determinare il timestamp del campione finale
    // ha zero senso se i campioni sono troppo distanziati nel tempo
    // alternativamente si può prendere il timestamp più recente
    uv_raw_mean /= sample_count;
    parsed_sample.uv_raw = uv_raw_mean;
    parsed_sample.uv_index = (float)(uv_raw_mean * ADC_CONVERSION_FACTOR) / VOLTAGE_INDEX_FACTOR;
    if (parsed_sample.uv_index < UV_TRESH)
    {
        parsed_sample.is_there_light = false;
    }
    else
        parsed_sample.is_there_light = true;

    // standard deviation characterization with less than 10 samples would be useless
    // validating based on the difference between the smaller and bigger sample could be a fix
    // but it might be too susceptible to noise
    if (sample_count < 10)
    {
        parsed_sample.s.valid == true;
        return parsed_sample;
    }

    float sample_variance = 0.0f;
    for (size_t i = 0; i < sample_count; i++)
    {
        sample_variance += pow((data[i].uv_raw - uv_raw_mean), 2.0);
        if (i == (sample_count - 1))
        {
            sample_variance /= (float)(sample_count - 1);
            if (sqrt(sample_variance) < MAX_DEV)
            {
                parsed_sample.s.valid = true;
            }
            else
                parsed_sample.s.valid = false;
        }
    }

    return parsed_sample;
}
