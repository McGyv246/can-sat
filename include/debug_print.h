#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

/*
Debug prints on the Arduino Serial port, enabled at compile time with build flags
(set them in platformio.ini, NOT in config.h, so the gcc tests never see them):
- SERIAL_DEBUG_ERRORS:   error messages (DBG_ERR)
- SERIAL_DEBUG_INFO:     events like flight state changes (DBG_INFO)
- SERIAL_DEBUG_DATA:     data dumps (DBG_DATA)
- SERIAL_DEBUG_COMMANDS: commands read from Serial for bench tests (see main.cpp)
- SERIAL_DEBUG:          shortcut that enables all of them
When disabled the macros expand to nothing, so they can be used in any file, even the tested ones.

DBG_ERR / DBG_INFO can't format floats: %f prints nothing on the board. Print them as integers
(e.g. millivolts, or value * 10 split with / and %), or use DBG_DATA for whole structs.

Every line starts with the kind, the time and the core that printed it,
e.g. "[ERR 12345ms c1] task SD failed". Lines are never cut by the other core's output.
*/

#ifdef SERIAL_DEBUG
#ifndef SERIAL_DEBUG_ERRORS
#define SERIAL_DEBUG_ERRORS
#endif
#ifndef SERIAL_DEBUG_INFO
#define SERIAL_DEBUG_INFO
#endif
#ifndef SERIAL_DEBUG_DATA
#define SERIAL_DEBUG_DATA
#endif
#ifndef SERIAL_DEBUG_COMMANDS
#define SERIAL_DEBUG_COMMANDS
#endif
#endif

#if defined(SERIAL_DEBUG_ERRORS) || defined(SERIAL_DEBUG_INFO) || defined(SERIAL_DEBUG_DATA) || defined(SERIAL_DEBUG_COMMANDS)
#define SERIAL_DEBUG_ANY
#include "serial_print_types.h"
#endif

/* printf-style, prints "[ERR <millis>ms c<core>] <message>" followed by a newline */
#ifdef SERIAL_DEBUG_ERRORS
#define DBG_ERR(...) debug_log_error(__VA_ARGS__)
#else
#define DBG_ERR(...) ((void)0)
#endif

/* printf-style, prints "[INF <millis>ms c<core>] <message>" followed by a newline */
#ifdef SERIAL_DEBUG_INFO
#define DBG_INFO(...) debug_log_info(__VA_ARGS__)
#else
#define DBG_INFO(...) ((void)0)
#endif

/* Calls one of the print_* functions of serial_print_types.h, e.g. DBG_DATA(print_imu, &imu) */
#ifdef SERIAL_DEBUG_DATA
#define DBG_DATA(fn, ptr) fn(ptr)
#else
#define DBG_DATA(fn, ptr) ((void)0)
#endif

#endif
