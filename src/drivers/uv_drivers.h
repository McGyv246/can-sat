#ifndef UV_DRIVERS_H_
#define UV_DRIVERS_H_

#include<stdint.h>
#include<types.h>

//there is no implemented way of turning on/off this sensor
//sens_status_t uv_init(void);

sens_status_t uv_poll(uint32_t now_ms);

sens_status_t uv_get(light_t *data);

//there is no implemented way of turning on/off this sensor
//void uv_sleep(bool on);

#endif

