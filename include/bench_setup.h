#ifndef BENCH_SETUP_H
#define BENCH_SETUP_H

#include "types.h"

/*
Bench test of single sensors (BENCH_TEST build flag): init only the sensors under test and enable
only their tasks. Flight state profiles are never applied, so nothing else gets enabled.
*/

// Core 0 sensors, call it from setup() after sched_init
void bench_setup(scheduler_t *sched_core0);

// Core 1 (SD / LoRa), call it from setup1() after sched_init since core 1 owns its scheduler
void bench_setup1(scheduler_t *sched_core1);

#endif
