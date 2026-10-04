#include <stdint.h>
#include <TinyGPS++.h>
#include "types.h"
#include "gps_drivers.h"
#include "arduino_wrapper.h"
#include "config.h"
#include "config_arduino.h"

static const uint32_t MAX_SAMPLE_AGE = 2000; // in ms
static const uint32_t GPS_BAUD = 9600;
// the NEO-6M sends ~500 bytes of NMEA per second at 9600 baud and the core's default RX buffer is
// only 32 bytes: with a poll every 500 ms most of the sentences would be lost. 1024 covers >1 s
static const size_t RX_BUFFER_SIZE = 1024;
static const uint8_t MIN_SATS = 4;

static bool is_on = false;
static TinyGPSPlus gps;
static gps_t latest_sample;

// feeds every byte received so far to the NMEA parser
static void read_available(void)
{
    while (Serial1.available() > 0)
        gps.encode(Serial1.read());
}

// sets the command pin too, so gps_wake works even if gps_init was never called
static void power_on(void)
{
    wrapper_pin_mode(D_OUT_GPS_COMMAND, WRAPPER_OUTPUT);
    wrapper_digital_write(D_OUT_GPS_COMMAND, WRAPPER_HIGH);
    Serial1.setFIFOSize(RX_BUFFER_SIZE); // must be called before begin
    Serial1.begin(GPS_BAUD);
    is_on = true;
}

static bool has_fix(void)
{
    return gps.location.isValid() && (gps.location.age() < MAX_SAMPLE_AGE);
}

extern "C"
{
    sens_state_t gps_init(void)
    {
        if (!is_on)
            power_on();
        read_available();
        // some other checks, like hdop validity, could be used to ensure enough satellites are locked
        if (has_fix() && gps.satellites.isValid() && (gps.satellites.value() >= MIN_SATS))
            return SENS_STATE_READY;
        return SENS_STATE_NOT_READY;
    }

    sens_state_t gps_wake(void)
    {
        if (!is_on)
            power_on();
        return SENS_STATE_NOT_READY;
    }

    /*
    A new sample is made every time a GGA sentence arrives (once per second), even without a fix:
    sats and hdop are sent anyway, so it's possible to see the GPS working while it searches.
    Without a fix s.valid is false and the position fields are NAN.
    */
    sens_state_t gps_poll(uint32_t now_ms)
    {
        if (!is_on)
            return SENS_STATE_ERR;

        read_available();
        if (!gps.satellites.isUpdated())
            return SENS_STATE_NOT_READY; // no new GGA sentence since the last sample

        bool fix = has_fix();

        latest_sample.sats = (uint8_t)gps.satellites.value();
        latest_sample.hdop = gps.hdop.isValid() ? (float)gps.hdop.hdop() : NAN;
        // GGA fix quality: 0 no fix, 1 GPS, 2 DGPS, ... (see TinyGPSLocation::Quality)
        latest_sample.fix_type = fix ? (uint8_t)(gps.location.FixQuality() - '0') : 0;

        if (fix)
        {
            latest_sample.lat = gps.location.lat();
            latest_sample.lon = gps.location.lng();
        }
        else
        {
            latest_sample.lat = NAN;
            latest_sample.lon = NAN;
        }

        if (gps.altitude.isValid() && (gps.altitude.age() < MAX_SAMPLE_AGE))
            latest_sample.alt_m = (float)gps.altitude.meters();
        else
            latest_sample.alt_m = NAN;

        // speed and course come from the RMC sentence
        if (gps.speed.isValid() && (gps.speed.age() < MAX_SAMPLE_AGE))
            latest_sample.speed_ms = (float)gps.speed.mps();
        else
            latest_sample.speed_ms = NAN;

        if (gps.course.isValid() && (gps.course.age() < MAX_SAMPLE_AGE))
            latest_sample.course_deg = (float)gps.course.deg();
        else
            latest_sample.course_deg = NAN;

        if (gps.time.isValid() && (gps.time.age() < MAX_SAMPLE_AGE))
            latest_sample.utc_s = (uint32_t)gps.time.hour() * 3600u + (uint32_t)gps.time.minute() * 60u + gps.time.second();
        else
            latest_sample.utc_s = 0;

        latest_sample.s.valid = fix && (latest_sample.sats >= MIN_SATS);
        latest_sample.s.t_ms = now_ms;
        return SENS_STATE_READY;
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
        }
        return SENS_STATE_NOT_READY;
    }
}
