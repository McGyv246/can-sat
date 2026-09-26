#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "types.h"

    void buf_write(void *data, size_t len);
    size_t buf_read(uint8_t *out);

    // ONLY for testing purposes
    void buf_reset();

#ifdef __cplusplus
}
#endif

#endif