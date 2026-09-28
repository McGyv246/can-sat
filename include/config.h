#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#define RESCHEDULE_FAULT_INTERVAL 1000

// Must be power of 2 and multiple of 512
#ifndef BUF_SIZE
// This if is necessary so that I can override this macro in tests
// by setting it in the compile command.
#define BUF_SIZE 32768
#endif

#define SYNC_WORD 0xA5A5

    // SAMPLE RATES (to be defined)

    // THRESHOLDS (such as ones to be used to determine flight state) (to be defined)

#ifdef __cplusplus
}
#endif

#endif