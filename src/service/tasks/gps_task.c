#include <stdint.h>

#include "gps_drivers.h"
#include "config.h"
#include "types.h"
#include "datastore.h"
#include "ring_buffer.h"

uint32_t gps_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = gps_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        gps_t data;
        gps_get(&data);

        ds_write_gps(&data, now_millis);
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

void gps_task_on_enable(task_t *self)
{
    sens_state_t state = gps_wake();

    if (state == SENS_STATE_ERR)
    {
        self->fail_count += 1;
    }
}

void gps_task_on_disable(task_t *self)
{
    sens_state_t state = gps_sleep();

    if (state == SENS_STATE_ERR)
    {
        self->fail_count += 1;
    }
}
