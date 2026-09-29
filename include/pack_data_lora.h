#ifndef PACK_DATA_LORA_H
#define PACK_DATA_LORA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"

    void ds_to_lora(data_to_lora_t *out, datastore_t *data);

#ifdef __cplusplus
}
#endif

#endif