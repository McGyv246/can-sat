#include <stdint.h>
#include <MAX17043.h>
#include "types.h"
#include "arduino_wrapper.h"

const uint8_t BATTERY_TRESHOLD_PERCENTAGE = 5; // default is 4%. can be between 1 and 32
const uint32_t FUEL_GAUGE_WAIT_TIME_MS = 1000; // don't think reading more than once a second could be beneficial...

static sens_internal_state_t state = SENS_INTERNAL_IDLE;
static uint32_t last_sample_time = 0;
static battery_t current_sample;

sens_external_state_t fuel_gauge_sleep(void)
{
    if (FuelGauge.isSleeping())
        return SENS_EXTERNAL_NOT_READY;
    if (!FuelGauge.sleep())
        return SENS_EXTERNAL_ERR;
    return SENS_EXTERNAL_NOT_READY;
}

void fuel_gauge_rebase_measures(void)
{
    FuelGauge.quickstart();
    return;
}

sens_external_state_t fuel_gauge_init(void)
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
            return SENS_EXTERNAL_ERR;
    }
    FuelGauge.threshold(BATTERY_TRESHOLD_PERCENTAGE);
    fuel_gauge_sleep();
    return SENS_EXTERNAL_NOT_READY;
}

sens_external_state_t fuel_gauge_wake()
{
    if (FuelGauge.isSleeping())
        if (!FuelGauge.wake())
            return SENS_EXTERNAL_ERR;
    return SENS_EXTERNAL_NOT_READY;
}

sens_external_state_t fuel_gauge_poll(uint32_t now_ms)
{
    switch (state)
    {
    case SENS_INTERNAL_IDLE:
        if ((now_ms - last_sample_time) < FUEL_GAUGE_WAIT_TIME_MS)
            return SENS_EXTERNAL_NOT_READY;
        current_sample.voltage_raw = FuelGauge.adc();
        current_sample.voltage = FuelGauge.voltage();
        current_sample.percentage = FuelGauge.percent();
        current_sample.is_alert_active = FuelGauge.alertIsActive();
        if (current_sample.is_alert_active)
            FuelGauge.clearAlert();
        state = SENS_INTERNAL_OK;

    case SENS_INTERNAL_OK:
        current_sample.s.t_ms = now_ms;
        current_sample.s.valid = true;
        state = SENS_INTERNAL_IDLE;
        return SENS_EXTERNAL_READY;
    }
    return SENS_EXTERNAL_ERR;
}

sens_external_state_t fuel_gauge_get(battery_t *data)
{
    *data = current_sample;
    return SENS_EXTERNAL_NOT_READY;
}
