#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include "types.h"
#include "debug_print.h"

/*
Commands read from Serial for bench tests (see debug_console.cpp for the list), plus the periodic
datastore print. Enabled by SERIAL_DEBUG_COMMANDS: without it these are empty stubs, so the caller
doesn't need any #ifdef.
*/

#ifdef SERIAL_DEBUG_COMMANDS

// Gives the console the schedulers the p / t / T commands work on. Call it after sched_init on core 0
void debug_console_init(scheduler_t *core0, scheduler_t *core1);

// Core 0, after every sched_tick: applies the fake sensor values, runs the commands and prints the datastore
void debug_console_poll_core0(void);

// Core 1, before every sched_tick: applies the task toggle requested by the T command
void debug_console_poll_core1(void);

#else

static inline void debug_console_init(scheduler_t *core0, scheduler_t *core1)
{
    (void)core0;
    (void)core1;
}
static inline void debug_console_poll_core0(void) {}
static inline void debug_console_poll_core1(void) {}

#endif

#endif
