#include <stdint.h>

#include "lora_drivers.h"
#include "config.h"
#include "types.h"
#include "tasks.h"
#include "ring_buffer.h"
#include "pack_data_lora.h"
#include "datastore.h"
#include "debug_print.h"

uint32_t lora_task_run(task_t *self, uint32_t now_millis)
{
    datastore_t ds;
    ds_read_all(&ds);

    data_to_lora_t data_to_send;
    ds_to_lora(&data_to_send, &ds);

    sens_state_t state = lora_send_packet((uint8_t *)&data_to_send, sizeof(data_to_send));

    if (state == SENS_STATE_ERR)
    {
        self->fail_count += 1;
        self->last_state = TASK_STATE_ERR;
        DBG_ERR("LORA send failed (seq %u)", (unsigned)data_to_send.seq);
    }
    else // SENS_STATE_READY || SENS_STATE_NOT_READY
    {
        self->last_state = TASK_STATE_BUSY;
        if (state == SENS_STATE_READY)
            DBG_INFO("LORA sent seq %u (%u B)", (unsigned)data_to_send.seq, (unsigned)sizeof(data_to_send));
        else /* previous packet still in the air (no TX done on DIO0) or duty cycle wait */
            DBG_INFO("LORA skipped seq %u: radio busy or duty cycle wait", (unsigned)data_to_send.seq);
    }

    /*
    Don't really know how to handle things state thing here,
    this task will never be OK, but there is no reason for me to
    distinguish between the two cases so yeah.
    */

    return self->period;
}

void lora_task_on_enable(task_t *self)
{
    sens_state_t state = lora_wake();
    if (state == SENS_STATE_ERR)
        self->fail_count += 1;
}

void lora_task_on_disable(task_t *self)
{
    lora_sleep();
}