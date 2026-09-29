#ifndef CHANGE_MISSION_STATE_TASK_H
#define CHANGE_MISSION_STATE_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "types.h"

    uint32_t run(task_t *self, uint32_t now_millis);

#ifdef __cplusplus
}
#endif

#endif