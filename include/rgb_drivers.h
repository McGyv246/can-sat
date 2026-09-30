#ifndef RGB_DRIVERS_H
#define RGB_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"

    // this function is to be called once on setup to set the pins
    void rgb_init(void);

    void rgb_set_color(rgb_colors_t color);

    // turns all rgb pins to low
    void rgb_sleep(void);

#ifdef __cplusplus
}
#endif

#endif
