#ifndef GYBMEP_DRIVERS_H_
#define GYBMEP_DRIVERS_H_

#include<stdint.h>
#include<types.h>

sens_status_t gybmep_init(void);

sens_status_t gybmep_poll(uint32_t now_ms);

sens_status_t gybmep_get(env_t *data);

sens_status_t gybmep_sleep(void);

#endif