#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void buf_write(void *data, size_t len);
    void buf_read(uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif