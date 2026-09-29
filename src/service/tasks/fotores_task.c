#include <stdint.h>

#include "config.h"
#include "types.h"
#include "fotores_drivers.h"
#include "datastore.h"
#include "ring_buffer.h"

uint32_t run(task_t *self, uint32_t now_millis)
{
    sens_state_t state = fotores_poll(now_millis);

    if (state == SENS_STATE_READY)
    {
        fotores_t data;
        fotores_get(&data);

        ds_write_fotores(&data, now_millis);
        buf_write(&data, sizeof(data));

        self->last_state = TASK_STATE_OK;
    }
    else
    { // for this sensor only SENS_STATE_ERR is possible
        self->fail_count += 1;
        self->last_state = TASK_STATE_ERR;
    }

    return self->period;
}