#include <stdint.h>
#include <MAX17043.h>
#include "types.h"
#include "fuel_gauge_drivers.h"
#include "arduino_wrapper.h"
#include "debug_print.h"

/*
Works with the MAX17043 and the MAX17048: VCELL and SOC have the same format on both.
On the MAX17048 sleep() does nothing unless MODE.EnSleep is set, so it just keeps measuring (harmless).
The chip is powered by the battery cell: without the battery it doesn't answer on I2C.
*/
static const uint8_t BATTERY_TRESHOLD_PERCENTAGE = 10; // default is 4%. can be between 1 and 32
static const uint32_t FUEL_GAUGE_WAIT_TIME_MS = 1000;  // don't think reading more than once a second could be beneficial...
// the gauge has no current sensing: at power up it guesses the SOC from the cell voltage, and if that
// happens under load (boot, brownouts) the guess is too low and the percentage keeps rising for a long time.
// init redoes the guess with a quickstart, after letting the voltage settle with the big loads (GPS, heater) off
static const uint32_t SETTLE_TIME_MS = 500;
static const uint32_t QUICKSTART_TIME_MS = 250; // time for the new estimate to be ready

static sens_state_t state = SENS_STATE_IDLE;
static uint32_t last_sample_time = 0;
static battery_t current_sample;

extern "C"
{
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
        // begin() starts Wire and returns false if the chip doesn't answer
        if (!FuelGauge.begin())
        {
            DBG_ERR("FUEL_GAUGE no answer (is the battery connected?)");
            return SENS_STATE_ERR;
        }

        FuelGauge.threshold(BATTERY_TRESHOLD_PERCENTAGE);

        // must run while nothing heavy is on: call fuel_gauge_init before turning on the GPS or the MQ-7 heater
        wrapper_delay_ms(SETTLE_TIME_MS);
        fuel_gauge_rebase_measures();
        wrapper_delay_ms(QUICKSTART_TIME_MS);
        // the log printf can't format floats (%f prints nothing): integers only
        unsigned mv = (unsigned)FuelGauge.voltage();
        unsigned pct_x10 = (unsigned)(FuelGauge.percent(true) * 10.0f);
        DBG_INFO("FUEL_GAUGE quickstart done: %u mV, %u.%u %%", mv, pct_x10 / 10, pct_x10 % 10);

        fuel_gauge_sleep();
        return SENS_STATE_NOT_READY;
    }

    sens_state_t fuel_gauge_wake()
    {
        state = SENS_STATE_IDLE;
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
            last_sample_time = now_ms;
            // the library doesn't check its reads: without this a missing chip gives garbage marked as valid
            if (!FuelGauge.deviceFound())
            {
                current_sample.s.valid = false;
                return SENS_STATE_ERR;
            }
            current_sample.voltage_raw = FuelGauge.adc();
            current_sample.voltage = FuelGauge.voltage() / 1000.0f; // the library gives mV
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

    void fuel_gauge_get(battery_t *data)
    {
        *data = current_sample;
        return;
    }
}
