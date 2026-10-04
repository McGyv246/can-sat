#ifndef THERMAL_SUMMARY_H
#define THERMAL_SUMMARY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"

    // Computes min, max, mean and hottest pixel of a frame, skipping NaN pixels.
    // out->s is copied from the frame, but valid is false if no pixel is a number
    void thermal_summarize(thermal_summary_t *out, const thermal_t *frame);

#ifdef __cplusplus
}
#endif

#endif
