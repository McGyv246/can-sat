#define _POSIX_C_SOURCE 199309L // Necessario per far poter usare CLOCK_MONOTONIC

#include <time.h>
#include <stdint.h>

// Nota che questa funzione in WSL ritornerà quanti nanosecondi sono
// passati dall'accensione e non il tempo che in generale i linguaggi di
// programmazione come Java o Python ritornano.
static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint32_t hal_millis(void)
{
    return (uint32_t)(now_ns() / 1000000ULL);
}

uint64_t hal_micros(void)
{
    return now_ns() / 1000ULL;
}
