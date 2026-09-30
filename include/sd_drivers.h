#ifndef SD_DRIVERS_H
#define SD_DRIVERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include "types.h"

    // initializes the sd module and creates a working directory or path of directories. path has to be a C-string type
    // so that string length is not needed
    sens_state_t sd_init(const char *path, const char *filepath);

    sens_state_t sd_write_uint8_buf(uint8_t *buf, size_t len);

    // ensures all data has been written to the sd. not sure if this should go directly in the write function
    sens_state_t sd_flush_buf(void);

    void sd_close(void);

#ifdef __cplusplus
}
#endif

#endif