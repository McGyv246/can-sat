#include <stddef.h>
#include "types.h"
#include "thermal_summary.h"

void thermal_summarize(thermal_summary_t *out, const thermal_t *frame)
{
    *out = (thermal_summary_t){.s = frame->s};

    size_t count = 0;
    float sum = 0.0f;

    for (size_t i = 0; i < THERMAL_PIXELS; i++)
    {
        float c = frame->pixels[i];
        if (c != c) // NaN is the only number that is not equal to itself
            continue;

        if (count == 0 || c < out->min_c)
            out->min_c = c;
        if (count == 0 || c > out->max_c)
        {
            out->max_c = c;
            out->hottest_px = (uint16_t)i;
        }
        sum += c;
        count++;
    }

    if (count == 0)
    {
        out->s.valid = false;
        return;
    }
    out->mean_c = sum / (float)count;
}
