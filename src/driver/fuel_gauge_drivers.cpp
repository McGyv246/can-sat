#include <stdint.h>
#include <MAX17043.h>
#include "types.h"
#include "arduino_wrapper.h"

const uint8_t BATTERY_TRESHOLD_PERCENTAGE = 5; // default is 4%. can be between 1 and 32
const uint32_t FUEL_GAUGE_WAIT_TIME_MS = 1000; // don't think reading more than once a second could be beneficial...

static sens_state_t state = SENS_STATE_IDLE;
static uint32_t last_sample_time = 0;
static battery_t current_sample;

sens_state_t fuel_gauge_sleep(void)
{
    if (FuelGauge.isSleeping())
        return SENS_STATE_NOT_READY;
    if (!FuelGauge.sleep())
        return SENS_STATE_ERR;
    return SENS_STATE_NOT_READY;
}

void fuel_gauge_rebase_measures(void)
{
    FuelGauge.quickstart();
    return;
}

sens_state_t fuel_gauge_init(void)
{
    FuelGauge.begin();
    wrapper_delay_ms(125);
    if (!FuelGauge.deviceFound())
    {
        uint8_t address = FuelGauge.findFirstDevice();
        if (address != 0)
        {
            FuelGauge.address(address);
            FuelGauge.reset();
            wrapper_delay_ms(250);
            fuel_gauge_rebase_measures();
            wrapper_delay_ms(125);
        }
        else
            return SENS_STATE_ERR;
    }
    FuelGauge.threshold(BATTERY_TRESHOLD_PERCENTAGE);
    fuel_gauge_sleep();
    return SENS_STATE_NOT_READY;
}

sens_state_t fuel_gauge_wake()
{
    if (FuelGauge.isSleeping())
        if (!FuelGauge.wake())
            return SENS_STATE_ERR;
    return SENS_STATE_NOT_READY;
}

sens_state_t fuel_gauge_poll(uint32_t now_ms)
{
    switch (state)
    {
    case SENS_STATE_IDLE:
        if ((now_ms - last_sample_time) < FUEL_GAUGE_WAIT_TIME_MS)
            return SENS_STATE_NOT_READY;
        current_sample.voltage_raw = FuelGauge.adc();
        current_sample.voltage = FuelGauge.voltage();
        current_sample.percentage = FuelGauge.percent();
        current_sample.is_alert_active = FuelGauge.alertIsActive();
        if (current_sample.is_alert_active)
            FuelGauge.clearAlert();
        state = SENS_STATE_READY;

    case SENS_STATE_READY:
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_STATE_IDLE;
        return SENS_STATE_READY;
    }
    return SENS_STATE_ERR;
}

sens_state_t fuel_gauge_get(battery_t *data)
{
    *data = current_sample;
    return SENS_STATE_NOT_READY;
}
