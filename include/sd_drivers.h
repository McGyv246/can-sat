#ifndef SD_DRIVERS_H_
#define SD_DRIVERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include "types.h"

    // initializes the sd module and creates a working directory or path of directories. path has to be a C-string type
    // so that string length is not needed
    sens_status_t sd_init(const char *path);

    // opens (or creates if it doesn't exist) the corresponding file
    sens_status_t sd_open_file(const char *filepath);

    sens_status_t sd_write_uint8_buf(uint8_t *buf, size_t len);

    // ensures all data has been written to the sd. not sure if this should go directly in the write function
    sens_status_t sd_flush_buf(void);

#ifdef __cplusplus
}
#endif

#endif