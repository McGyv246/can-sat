#include <stdint.h>

#include "mq_7_drivers.h"
#include "config.h"
#include "types.h"
#include "datastore.h"
#include "ring_buffer.h"
#include "hal_time.h"

uint32_t mq_7_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = mq_7_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        gas_t data;
        gas_get(&data);

        ds_write_gas(&data, now_millis);
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

void mq_7_task_on_enable(task_t *self)
{
    mq_7_wake(hal_millis()); // It might be better to make this in some other way, but don't care
}

void mq_7_task_on_disable(task_t *self)
{
    mq_7_sleep();
}
