#include <stdint.h>

#include "sd_drivers.h"
#include "config.h"
#include "types.h"
#include "tasks.h"
#include "datastore.h"
#include "ring_buffer.h"

static uint8_t array_buf_read[BUF_SIZE];

uint32_t sd_task_run(task_t *self, uint32_t now_millis)
{
    if (self->last_state == TASK_STATE_BUSY)
    {
        sens_state_t state = sd_flush_buf();

        if (state == SENS_STATE_ERR)
        {
            self->fail_count += 1;
            self->last_state = TASK_STATE_ERR;
        }
        else // SENS_STATE_READY
        {
            self->last_state = TASK_STATE_OK;
        }
    }
    else // TASK_STATE_OK || TASK_STATE_ERR
    {
        size_t len = buf_read(array_buf_read);

        sens_state_t state = sd_write_uint8_buf(array_buf_read, len);

        if (state == SENS_STATE_ERR)
        {
            self->fail_count += 1;
            self->last_state = TASK_STATE_ERR;
        }
        else if (state == SENS_STATE_NOT_READY)
        {
            self->last_state = TASK_STATE_BUSY;
        }
        else // SENS_STATE_OK
        {
            // This shouldn't happen
            self->last_state = TASK_STATE_OK;
        }
    }

    return self->period;
}