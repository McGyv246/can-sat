#include <stdint.h>

#include "uv_drivers.h"
#include "config.h"
#include "types.h"
#include "datastore.h"
#include "ring_buffer.h"

uint32_t uv_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = uv_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        uv_t data;
        uv_get(&data);

        ds_write_uv(&data, now_millis);
        buf_write(&data, sizeof(data));

        self->last_state = TASK_STATE_OK;
    }
    else if (state == SENS_STATE_NOT_READY)
    {
        self->last_state = TASK_STATE_BUSY;
        // TODO: eventually add another return here for
        // different reschedule interval in case it's busy
    }
    else
    { // for this sensor only SENS_STATE_ERR is possible
        self->fail_count += 1;
        self->last_state = TASK_STATE_ERR;
    }

    return self->period;
}
