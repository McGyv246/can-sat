#include <stdint.h>
#include "types.h"
#include "datastore.h"

uint32_t run(task_t *self, uint32_t now_millis)
{
    datastore_t ds;
    ds_read_all(&ds);

    switch (ds.state)
    {
    case FLIGHT_STATE_BOOT:
        /*
        Don't really know what's necessary here, likely nothing
        the scheduler shouldn't run in this state.
        Likely it's better to run all sensor's init functions and just
        change the state to the next one.
        Maybe get the gps on until it streams data so that I know that it locked satellites.
        */
        break;
    case FLIGHT_STATE_SELF_TEST:
        /*
        The same of above applies here, likely it's a thing that should
        be done just on startup.
        For now it's not even implemented yet any self test
        */
        break;
    case FLIGHT_STATE_IDLE:
        /*
        Should poll photoresistor and uv quite frequently,
        detect if there are spikes in light for a few seconds
        and then pass onto the descent step.
        Maybe it could even check for accelerations and eventually
        pass to the ascent step, but it might not be worth it since the
        ascent only lasts a few seconds.
        */
        break;
    case FLIGHT_STATE_ASCENT:
        /*
        Should poll the photoresistor and uv to detect ejection.
        */
        break;
    case FLIGHT_STATE_DESCENT:
        /*
        Lower polling rate for photoresistor and uv, enable every other sensor.
        Detect sudden stop of accelerations, rotations, gps position, altitude
        to detect when the CanSat lands
        */
        break;
    case FLIGHT_STATE_LANDED:
        /*
        Disable / lower polling rate for everything.
        Keep sending position over LoRa.
        */
        break;

    default:
        break;
    }
}