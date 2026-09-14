#ifndef GYBMEP_DRIVERS_H_
#define GYBMEP_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include<stdint.h>
#include<types.h>

//checks if the address and chip_id of the sensor are correct. this function is to be called once
//to check everything is in order
sens_status_t gybmep_setup(void);

sens_status_t gybmep_init(void);

sens_status_t gybmep_poll(uint32_t now_ms);

sens_status_t gybmep_get(env_t *data);

sens_status_t gybmep_sleep(void);

#ifdef __cplusplus
}
#endif

#endif