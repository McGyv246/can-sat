#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <TinyGPS++.h>
#include "types.h"
#include "gps_drivers.h"
#include "arduino_wrapper.h"
#include "config.h"
#include "config_arduino.h"
#include "debug_print.h"

static const uint32_t MAX_SAMPLE_AGE = 2000; // in ms
static const uint32_t GPS_BAUD = 9600;
// the NEO-6M sends ~500 bytes of NMEA per second at 9600 baud and the core's default RX buffer is
// only 32 bytes: with a poll every 500 ms most of the sentences would be lost. 1024 covers >1 s
static const size_t RX_BUFFER_SIZE = 1024;
static const uint8_t MIN_SATS = 4;

static bool is_on = false;
static TinyGPSPlus gps;
static gps_t latest_sample;

#pragma region debug

// Sentences TinyGPS++ doesn't parse by itself, only used by gps_get_debug
static TinyGPSCustom gsa_mode(gps, "GPGSA", 2);
static TinyGPSCustom gsv_msg_count(gps, "GPGSV", 1);
static TinyGPSCustom gsv_msg_num(gps, "GPGSV", 2);
static TinyGPSCustom gsv_in_view(gps, "GPGSV", 3);
// every GSV message has up to 4 satellites, each one as PRN, elevation, azimuth, SNR
static TinyGPSCustom gsv_prn[4] = {{gps, "GPGSV", 4}, {gps, "GPGSV", 8}, {gps, "GPGSV", 12}, {gps, "GPGSV", 16}};
static TinyGPSCustom gsv_elev[4] = {{gps, "GPGSV", 5}, {gps, "GPGSV", 9}, {gps, "GPGSV", 13}, {gps, "GPGSV", 17}};
static TinyGPSCustom gsv_snr[4] = {{gps, "GPGSV", 7}, {gps, "GPGSV", 11}, {gps, "GPGSV", 15}, {gps, "GPGSV", 19}};
static TinyGPSCustom txt(gps, "GPTXT", 4);

static gps_debug_t dbg;
static uint8_t gsv_prn_work[GPS_DEBUG_MAX_SATS]; // GSV cycle being collected, copied to dbg when complete
static uint8_t gsv_snr_work[GPS_DEBUG_MAX_SATS];
static int8_t gsv_elev_work[GPS_DEBUG_MAX_SATS];
static uint8_t gsv_work_count;
static uint32_t power_on_ms;

// NMEA lines are at most 82 chars, longer ones are cut
static bool raw_echo = false;
static char raw_line[96];
static size_t raw_len;

/*
UBX binary messages (u-blox protocol), only the ones used for debugging:
  b5 62 <class> <id> <len LE16> <payload> <ck_a> <ck_b>
The replies arrive mixed with the NMEA: 0xb5 is never in NMEA (ASCII only), so it starts a UBX frame
and its bytes go to the UBX parser instead of TinyGPS++.
*/
#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62
#define UBX_CLASS_ACK 0x05
#define UBX_ID_ACK_NAK 0x00
#define UBX_ID_ACK_ACK 0x01
#define UBX_CLASS_CFG 0x06
#define UBX_ID_CFG_ITFM 0x39
#define UBX_ID_CFG_RST 0x04
#define UBX_CLASS_MON 0x0A
#define UBX_ID_MON_HW 0x09
#define UBX_MAX_PAYLOAD 72 // MON-HW: 68 bytes on u-blox 6, 60 on u-blox 7/8

typedef enum
{
    UBX_IDLE,
    UBX_SYNC,
    UBX_CLASS,
    UBX_ID,
    UBX_LEN1,
    UBX_LEN2,
    UBX_PAYLOAD,
    UBX_CK_A,
    UBX_CK_B,
} ubx_state_t;

static ubx_state_t ubx_state = UBX_IDLE;
static uint8_t ubx_class, ubx_id, ubx_ck_a, ubx_ck_b;
static uint16_t ubx_len, ubx_pos;
static uint8_t ubx_payload[UBX_MAX_PAYLOAD];

// Called after every good sentence: GSV is split in many messages, the values of each one must be
// read before the next one overwrites them
static void debug_on_sentence(void)
{
    if (gsa_mode.isUpdated())
        dbg.fix_mode = (uint8_t)atoi(gsa_mode.value());

    if (txt.isUpdated())
    {
        const char *t = txt.value();
        if (strncmp(t, "u-blox ag", 9) == 0) // first line of the boot banner
            dbg.boots++;
        else if (strncmp(t, "ANTSTATUS=", 10) == 0)
        {
            // TinyGPSCustom keeps 14 chars, so "ANTSTATUS=DONTKNOW" arrives as "ANTSTATUS=DONT"
            const char *status = strcmp(t + 10, "DONT") == 0 ? "DONTKNOW" : t + 10;
            strncpy(dbg.antenna, status, sizeof(dbg.antenna) - 1);
            dbg.antenna[sizeof(dbg.antenna) - 1] = '\0';
        }
    }

    if (!gsv_msg_num.isUpdated())
        return;

    int num = atoi(gsv_msg_num.value());
    int count = atoi(gsv_msg_count.value());
    int in_view = atoi(gsv_in_view.value());
    if (num == 1)
        gsv_work_count = 0;

    // The last message can have less than 4 satellites. Its missing fields keep the values of the
    // previous message in TinyGPSCustom, so how many to read comes from the satellites in view
    int in_this_msg = in_view - (num - 1) * 4;
    if (in_this_msg > 4)
        in_this_msg = 4;

    for (int i = 0; i < in_this_msg && gsv_work_count < GPS_DEBUG_MAX_SATS; i++)
    {
        gsv_prn_work[gsv_work_count] = (uint8_t)atoi(gsv_prn[i].value());
        gsv_snr_work[gsv_work_count] = (uint8_t)atoi(gsv_snr[i].value()); // empty = not tracked = 0
        const char *elev = gsv_elev[i].value();
        gsv_elev_work[gsv_work_count] = elev[0] ? (int8_t)atoi(elev) : -1; // empty = orbit not known yet
        gsv_work_count++;
    }

    if (num == count)
    {
        dbg.sats_in_view = (uint8_t)in_view;
        memcpy(dbg.prn, gsv_prn_work, gsv_work_count);
        memcpy(dbg.snr, gsv_snr_work, gsv_work_count);
        memcpy(dbg.elev, gsv_elev_work, gsv_work_count);
        dbg.sats_count = gsv_work_count;
    }
}

// Collects the bytes of a line and prints it when complete, if raw_echo is on
static void debug_echo_char(char c)
{
    if (c == '\r')
        return;
    if (c != '\n')
    {
        if (raw_len < sizeof(raw_line) - 1)
            raw_line[raw_len++] = c;
        return;
    }
    raw_line[raw_len] = '\0';
    raw_len = 0;
    if (raw_echo && raw_line[0] != '\0')
        DBG_INFO("GPS> %s", raw_line);
}

// 8-bit Fletcher checksum of the UBX protocol
static void ubx_checksum(uint8_t *ck_a, uint8_t *ck_b, const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        *ck_a += data[i];
        *ck_b += *ck_a;
    }
}

static void ubx_checksum_add(uint8_t b) { ubx_checksum(&ubx_ck_a, &ubx_ck_b, &b, 1); }

// Own checksum variables: a reply may be half parsed when a request is sent
static void ubx_send(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len)
{
    uint8_t head[6] = {UBX_SYNC1, UBX_SYNC2, cls, id, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8)};
    uint8_t ck[2] = {0, 0};
    ubx_checksum(&ck[0], &ck[1], head + 2, 4);
    ubx_checksum(&ck[0], &ck[1], payload, len);

    Serial1.write(head, sizeof(head));
    if (len)
        Serial1.write(payload, len);
    Serial1.write(ck, sizeof(ck));
}

static uint16_t read_u16_le(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static void ubx_on_message(void)
{
    if (ubx_class == UBX_CLASS_MON && ubx_id == UBX_ID_MON_HW && (ubx_len == 68 || ubx_len == 60))
    {
        dbg.hw_replies++;
        dbg.noise_per_ms = read_u16_le(ubx_payload + 16);
        dbg.agc_cnt = read_u16_le(ubx_payload + 18);
        dbg.jam_state = (ubx_payload[22] >> 2) & 0x03;
        // jamInd comes after the VP array, 25 bytes on u-blox 6 and 17 on u-blox 7/8
        dbg.jam_ind = ubx_payload[ubx_len == 68 ? 53 : 45];
    }
    else if (ubx_class == UBX_CLASS_ACK && ubx_len == 2 && ubx_payload[0] == UBX_CLASS_CFG && ubx_payload[1] == UBX_ID_CFG_ITFM)
    {
        dbg.itfm_ack = ubx_id == UBX_ID_ACK_ACK ? 2 : 3;
    }
}

// Returns true if c belongs to a UBX frame (so it must not go to the NMEA parser)
static bool ubx_parse(uint8_t c)
{
    switch (ubx_state)
    {
    case UBX_IDLE:
        if (c != UBX_SYNC1)
            return false;
        ubx_state = UBX_SYNC;
        return true;
    case UBX_SYNC:
        ubx_state = c == UBX_SYNC2 ? UBX_CLASS : UBX_IDLE;
        return true;
    case UBX_CLASS:
        ubx_ck_a = 0;
        ubx_ck_b = 0;
        ubx_checksum_add(c);
        ubx_class = c;
        ubx_state = UBX_ID;
        return true;
    case UBX_ID:
        ubx_checksum_add(c);
        ubx_id = c;
        ubx_state = UBX_LEN1;
        return true;
    case UBX_LEN1:
        ubx_checksum_add(c);
        ubx_len = c;
        ubx_state = UBX_LEN2;
        return true;
    case UBX_LEN2:
        ubx_checksum_add(c);
        ubx_len |= (uint16_t)c << 8;
        ubx_pos = 0;
        ubx_state = ubx_len ? UBX_PAYLOAD : UBX_CK_A;
        return true;
    case UBX_PAYLOAD:
        ubx_checksum_add(c);
        if (ubx_pos < UBX_MAX_PAYLOAD)
            ubx_payload[ubx_pos] = c; // longer payloads are checked but not kept (and not used)
        if (++ubx_pos == ubx_len)
            ubx_state = UBX_CK_A;
        return true;
    case UBX_CK_A:
        ubx_state = c == ubx_ck_a ? UBX_CK_B : UBX_IDLE;
        return true;
    case UBX_CK_B:
        ubx_state = UBX_IDLE;
        if (c == ubx_ck_b && ubx_len <= UBX_MAX_PAYLOAD)
            ubx_on_message();
        return true;
    }
    return false;
}

#pragma endregion

// feeds every byte received so far to the NMEA parser (UBX replies go to the debug parser)
static void read_available(void)
{
    if (Serial1.overflow())
        dbg.rx_overflows++;

    while (Serial1.available() > 0)
    {
        char c = (char)Serial1.read();
        if (ubx_parse((uint8_t)c))
            continue;
        debug_echo_char(c);
        if (gps.encode(c))
            debug_on_sentence();
    }
}

// sets the command pin too, so gps_wake works even if gps_init was never called
static void power_on(void)
{
    wrapper_pin_mode(D_OUT_GPS_COMMAND, WRAPPER_OUTPUT);
    wrapper_digital_write(D_OUT_GPS_COMMAND, WRAPPER_HIGH);
    Serial1.setFIFOSize(RX_BUFFER_SIZE); // must be called before begin
    Serial1.begin(GPS_BAUD);
    is_on = true;
    power_on_ms = millis();
    // the module lost the jamming monitor setting with the power
    dbg.hw_requests = 0;
    dbg.hw_replies = 0;
    dbg.itfm_ack = 0;
    ubx_state = UBX_IDLE;
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

    void gps_get_debug(gps_debug_t *d)
    {
        dbg.chars = gps.charsProcessed();
        dbg.passed_checksum = gps.passedChecksum();
        dbg.failed_checksum = gps.failedChecksum();
        dbg.time_valid = gps.time.isValid() && (gps.time.age() < MAX_SAMPLE_AGE);
        dbg.on_ms = is_on ? millis() - power_on_ms : 0;
        *d = dbg;
    }

    void gps_set_raw_echo(bool on)
    {
        raw_echo = on;
        raw_len = 0; // the line being collected may have started before
    }

    void gps_request_hw_status(void)
    {
        if (!is_on)
            return;

        // sent again until answered: right after the power on the module may still be booting
        if (dbg.itfm_ack < 2)
        {
            /*
            CFG-ITFM, values recommended by u-blox: bbThreshold 3, cwThreshold 15, the fixed algorithm
            bits 0x16B156 and enable (bit 31); config2 the fixed bits 0x31E, antenna type unknown
            */
            const uint32_t config = 3u | (15u << 4) | (0x16B156u << 9) | (1u << 31);
            const uint32_t config2 = 0x31Eu;
            uint8_t payload[8];
            for (int i = 0; i < 4; i++)
            {
                payload[i] = (uint8_t)(config >> (8 * i));
                payload[4 + i] = (uint8_t)(config2 >> (8 * i));
            }
            ubx_send(UBX_CLASS_CFG, UBX_ID_CFG_ITFM, payload, sizeof(payload));
            dbg.itfm_ack = 1; // until the ACK/NAK arrives
        }

        ubx_send(UBX_CLASS_MON, UBX_ID_MON_HW, NULL, 0);
        dbg.hw_requests++;
    }

    bool gps_cold_start(void)
    {
        if (!is_on)
            return false;

        // navBbrMask 0xFFFF = clear everything (cold start), resetMode 0x02 = software reset of the GPS
        // part only (the UART keeps working, no boot banner), the module doesn't answer it
        const uint8_t payload[4] = {0xFF, 0xFF, 0x02, 0x00};
        ubx_send(UBX_CLASS_CFG, UBX_ID_CFG_RST, payload, sizeof(payload));
        power_on_ms = millis();
        return true;
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
