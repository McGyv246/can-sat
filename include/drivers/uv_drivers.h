#ifndef UV_DRIVERS_H_
#define UV_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{

#include<stdint.h>
#include<types.h>

//uv_init switches the mode of the sensor from an accurate but slower sampling that requires ~10ms to a faster sampling
//set to sleep when descent start
sens_status_t uv_init(void);

sens_status_t uv_poll(uint32_t now_ms);

sens_status_t uv_get(light_t *data);

//uv_sleep switches the mode of the sensor from a faster sampling to an accurate but slower sampling that requires ~10ms
//set to sleep when in standby for launch
sens_status_t sleep(void);

#endif
}
#endif
