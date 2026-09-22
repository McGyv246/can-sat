#include <stdint.h>

#include "arduino_wrapper.h"

// #include "pico/time.h" // for time_us_64()

uint32_t hal_millis(void)
{
    return millis();
}

// uint64_t hal_micros(void)
// {
//     return time_us_64();
// }
