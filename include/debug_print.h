#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

/*
Debug prints on the Arduino Serial port, enabled at compile time with build flags
(set them in platformio.ini, NOT in config.h, so the gcc tests never see them):
- SERIAL_DEBUG_ERRORS: error messages (DBG_ERR)
- SERIAL_DEBUG_DATA:   data dumps (DBG_DATA)
- SERIAL_DEBUG:        shortcut that enables both
When disabled the macros expand to nothing, so they can be used in any file, even the tested ones.
*/

#ifdef SERIAL_DEBUG
#ifndef SERIAL_DEBUG_ERRORS
#define SERIAL_DEBUG_ERRORS
#endif
#ifndef SERIAL_DEBUG_DATA
#define SERIAL_DEBUG_DATA
#endif
#endif

#if defined(SERIAL_DEBUG_ERRORS) || defined(SERIAL_DEBUG_DATA)
#include "serial_print_types.h"
#endif

/* printf-style, prints "[ERR] <message>" followed by a newline */
#ifdef SERIAL_DEBUG_ERRORS
#define DBG_ERR(...) debug_log_error(__VA_ARGS__)
#else
#define DBG_ERR(...) ((void)0)
#endif

/* Calls one of the print_* functions of serial_print_types.h, e.g. DBG_DATA(print_imu, &imu) */
#ifdef SERIAL_DEBUG_DATA
#define DBG_DATA(fn, ptr) fn(ptr)
#else
#define DBG_DATA(fn, ptr) ((void)0)
#endif

#endif
