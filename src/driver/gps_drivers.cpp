#include <stdint.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "types.h"
#include "gps_drivers.h"
#include "arduino_wrapper.h"
#include "config.h"
#include "config_arduino.h"

static const uint32_t MAX_SAMPLE_AGE = 2000; // in ms

static bool is_on = false;
static sens_state_t state = SENS_STATE_IDLE;
static TinyGPSPlus gps;
static gps_t latest_sample;

extern "C"
{
    sens_state_t gps_init(void)
    {
        if (!is_on)
        {
            wrapper_pin_mode(D_OUT_GPS_COMMAND, WRAPPER_OUTPUT);
            wrapper_digital_write(D_OUT_GPS_COMMAND, WRAPPER_HIGH);
            Serial1.begin(9600);
            is_on = true;
            state = SENS_STATE_IDLE;
        }
        if (Serial1.available() > 0)
        {
            gps.encode(Serial1.read());
            // some other checks, like hdop validity, could be used to ensure enough satellites are locked
            if (gps.location.isValid() && (gps.location.age() < MAX_SAMPLE_AGE) && gps.satellites.isValid() && (gps.satellites.value() >= 4))
                return SENS_STATE_READY;
        }
        return SENS_STATE_NOT_READY;
    }

    sens_state_t gps_wake(void)
    {
        if (!is_on)
        {
            wrapper_digital_write(D_OUT_GPS_COMMAND, WRAPPER_HIGH);
            Serial1.begin(9600);
            is_on = true;
            state = SENS_STATE_IDLE;
        }
        return SENS_STATE_NOT_READY;
    }

    sens_state_t gps_poll(uint32_t now_ms)
    {
        switch (state)
        {
        case SENS_STATE_IDLE:
            if (Serial1.available())
            {
                state = SENS_STATE_READY;
            }
            else
            {
                return SENS_STATE_NOT_READY;
            }
        case SENS_STATE_READY:
            gps.encode(Serial1.read());

            if (gps.satellites.isValid() && (gps.satellites.age() < MAX_SAMPLE_AGE) && gps.satellites.isUpdated())
            {
                latest_sample.sats = gps.satellites.value();
            }
            else
            {
                latest_sample.sats = 0;
                latest_sample.s.valid = false;
            }

            if (gps.altitude.isValid() && (gps.altitude.age() < MAX_SAMPLE_AGE) && gps.altitude.isUpdated())
            {
                latest_sample.alt_m = gps.altitude.meters();
            }
            else
            {
                latest_sample.alt_m = NAN;
                latest_sample.s.valid = false;
            }

            if (gps.course.isValid() && (gps.course.age() < MAX_SAMPLE_AGE) && gps.course.isUpdated())
            {
                latest_sample.course_deg = gps.course.deg();
            }
            else
            {
                latest_sample.course_deg = NAN;
                latest_sample.s.valid = false;
            }

            if (gps.speed.isValid() && (gps.speed.age() < MAX_SAMPLE_AGE) && gps.speed.isUpdated())
            {
                latest_sample.speed_ms = gps.speed.mps();
            }
            else
            {
                latest_sample.speed_ms = NAN;
                latest_sample.s.valid = false;
            }

            if (gps.location.isValid() && (gps.location.age() < MAX_SAMPLE_AGE) && gps.location.isUpdated())
            {
                latest_sample.lat = gps.location.lat();
                latest_sample.lon = gps.location.lng();
            }
            else
            {
                latest_sample.lat = NAN;
                latest_sample.lon = NAN;
                latest_sample.s.valid = false;
            }

            if (gps.hdop.isValid() && (gps.hdop.age() < MAX_SAMPLE_AGE) && gps.hdop.isUpdated())
            {
                latest_sample.hdop = gps.hdop.value();
            }
            else
            {
                latest_sample.hdop = NAN;
                latest_sample.s.valid = false;
            }

            if (gps.time.isValid() && (gps.time.age() < MAX_SAMPLE_AGE) && gps.time.isUpdated())
            {
                latest_sample.utc_s = gps.time.value();
            }
            else
            {
                latest_sample.utc_s = 0;
                latest_sample.s.valid = false;
            }

            latest_sample.s.valid = true;
            latest_sample.s.t_ms = now_ms;
            state = SENS_STATE_IDLE;
            return SENS_STATE_READY;
        }
        return SENS_STATE_ERR;
    }

    void gps_get(gps_t *data)
    {
        *data = latest_sample;
        return;
    }

    sens_state_t gps_sleep(void)
    {
        if (is_on)
        {
            wrapper_digital_write(D_OUT_GPS_COMMAND, WRAPPER_LOW);
            Serial1.end();
            is_on = false;
            state = SENS_STATE_IDLE;
        }
        return SENS_STATE_NOT_READY;
    }
}