#include <stdint.h>
#include "types.h"
#include "uv_drivers.h"
#include "hal_time.h"

sens_external_state_t store_data();

uint32_t run(task_t *self, uint32_t now_millis)
{
    sens_external_state_t status = self->last_status;

    switch (status)
    {
    case SENS_EXTERNAL_NOT_READY:
        sens_external_state_t new_status = uv_poll(now_millis);
        if (new_status == SENS_EXTERNAL_READY)
        {
            new_status = store_data();
        }
        self->last_status = new_status;
        break;

    case SENS_EXTERNAL_READY:
        self->last_status = store_data();
        break;

    default: // Also catches case SENS_EXTERNAL_ERR
        /*
        I could handle errors by rescheduling after a time that increases with
        some proportion to the number of times this task failed.
        After a certain number the task gets a due date very far so that it doesn't retry.
        I should add a field for a cunter of times it failed in the task struct.
        */
        break;
    }
    return now_millis; // added this so that i could compile the code. change it if needed
}

sens_external_state_t store_data()
{
    /*
    Use uv_get to get the value and store it in the
    */
    return SENS_EXTERNAL_NOT_READY; // added this so that i could compile the code. change it if needed
}