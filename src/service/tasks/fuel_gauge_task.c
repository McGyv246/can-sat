#include <stdint.h>

#include "fuel_gauge_drivers.h"
#include "config.h"
#include "types.h"
#include "datastore.h"
#include "ring_buffer.h"

uint32_t fuel_gauge_task_run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = fuel_gauge_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        battery_t data;
        fuel_gauge_get(&data);

        ds_write_battery(&data, now_millis);
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

void fuel_gauge_task_on_enable(task_t *self)
{
    sens_state_t state = fuel_gauge_wake();

    if (state == SENS_STATE_ERR)
    {
        self->fail_count += 1; // if this happens we are fucked actually
    }
}

void fuel_gauge_task_on_disable(task_t *self)
{
    sens_state_t state = fuel_gauge_sleep();

    if (state == SENS_STATE_ERR)
    {
        self->fail_count += 1; // if this happens doesn't matter so much
    }
}