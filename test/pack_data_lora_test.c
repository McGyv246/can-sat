// compile: gcc -fsanitize=address -DUNITY_INCLUDE_DOUBLE -I include -I mocks -I test/unity -Wall test/unity/unity.c test/pack_data_lora_test.c src/codec/pack_data_lora.c src/codec/crc.c -lm -o build/pack_data_lora_runner

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "unity.h"
#include "types.h"
#include "config.h"
#include "crc.h"
#include "pack_data_lora.h"

static datastore_t ds;
static data_to_lora_t packet;
static lora_decoded_t decoded;

/* A datastore with realistic values for every field */
static void fill_datastore()
{
    ds.t_ms = 123456;
    ds.state = FLIGHT_STATE_DESCENT;
    ds.records_dropped = 7;

    ds.env.s.valid = true;
    ds.env.temp_c = 21.37f;
    ds.env.pressure_pa = 95432.0f;
    ds.env.humidity_pct = 45.5f;
    ds.env.altitude_m = 512.4f;

    ds.imu.s.valid = true;
    ds.imu.accel[0] = 0.012f;
    ds.imu.accel[1] = -0.981f;
    ds.imu.accel[2] = 3.5f;
    ds.imu.gyro[0] = 120.3f;
    ds.imu.gyro[1] = -45.6f;
    ds.imu.gyro[2] = 0.0f;
    ds.imu.accel_mag = 3.635f;

    ds.uv.s.valid = true;
    ds.uv.is_there_light = true;
    ds.uv.uv_index = 6.3f;

    ds.fotores.s.valid = true;
    ds.fotores.is_there_light = false;

    ds.gas.s.valid = true;
    ds.gas.raw = 812;
    ds.gas.heater_enabled = true;
    ds.gas.heater_on_s = 42;

    ds.gps.s.valid = true;
    ds.gps.lat = 45.4641943;
    ds.gps.lon = -9.1896346;
    ds.gps.alt_m = 530.0f;
    ds.gps.sats = 9;
    ds.gps.fix_type = 3;

    ds.battery.s.valid = true;
    ds.battery.voltage = 3.87f;
    ds.battery.is_alert_active = false;
}

static lora_decode_result_t pack_and_decode()
{
    ds_to_lora(&packet, &ds);
    return lora_decode(&decoded, (const uint8_t *)&packet, sizeof(packet));
}

/* Changes the packet as if the satellite had sent it like this: the CRC is recomputed */
static void refresh_crc()
{
    packet.crc16 = crc16((const uint8_t *)&packet, sizeof(packet) - sizeof(packet.crc16));
}

void setUp()
{
    memset(&ds, 0, sizeof(ds));
    memset(&packet, 0, sizeof(packet));
    memset(&decoded, 0, sizeof(decoded));
}

void tearDown() {}

#pragma region round_trip

void when_packetDecoded_expect_sameValuesWithinResolution()
{
    fill_datastore();

    TEST_ASSERT_EQUAL(LORA_DECODE_OK, pack_and_decode());

    TEST_ASSERT_EQUAL_UINT8(LORA_VERSION, decoded.version);
    TEST_ASSERT_EQUAL_UINT32(123456, decoded.ms_since_boot);
    TEST_ASSERT_EQUAL_UINT8(FLIGHT_STATE_DESCENT, decoded.flight_state);
    TEST_ASSERT_EQUAL_UINT16(7, decoded.records_dropped);

    // Tolerance = half the resolution of each field
    TEST_ASSERT_FLOAT_WITHIN(0.005f, 21.37f, decoded.temp_c);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 95432.0f, decoded.pressure_pa);
    TEST_ASSERT_FLOAT_WITHIN(0.25f, 45.5f, decoded.humidity_pct);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 512.4f, decoded.altitude_baro_m);

    TEST_ASSERT_FLOAT_WITHIN(0.0005f, 0.012f, decoded.accel[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0005f, -0.981f, decoded.accel[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0005f, 3.5f, decoded.accel[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 120.3f, decoded.gyro[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -45.6f, decoded.gyro[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, decoded.gyro[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.0005f, 3.635f, decoded.accel_mag);

    TEST_ASSERT_TRUE(decoded.is_there_light_uv);
    TEST_ASSERT_FALSE(decoded.is_there_light_fotores);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 6.3f, decoded.uv_index);

    TEST_ASSERT_EQUAL_UINT16(812, decoded.gas_raw);
    TEST_ASSERT_EQUAL_UINT8(42, decoded.heater_on_s);

    TEST_ASSERT_DOUBLE_WITHIN(0.5e-7, 45.4641943, decoded.lat);
    TEST_ASSERT_DOUBLE_WITHIN(0.5e-7, -9.1896346, decoded.lon);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 530.0f, decoded.altitude_gps_m);
    TEST_ASSERT_EQUAL_UINT8(9, decoded.sats);
    TEST_ASSERT_EQUAL_UINT8(3, decoded.fix_type);

    TEST_ASSERT_FLOAT_WITHIN(0.005f, 3.87f, decoded.vbatt);
}

void when_allBlocksValid_expect_allStatusBitsSet()
{
    fill_datastore();
    ds.battery.is_alert_active = true;

    pack_and_decode();

    uint16_t expected = STATUS_ENV_OK | STATUS_IMU_OK | STATUS_GPS_OK | STATUS_UV_OK | STATUS_FOTORES_OK |
                        STATUS_GAS_OK | STATUS_BATTERY_OK | STATUS_BATTERY_ALERT | STATUS_HEATER_ON;
    TEST_ASSERT_EQUAL_HEX16(expected, decoded.status);
}

void when_noBlockValid_expect_noStatusBitSet()
{
    pack_and_decode();

    TEST_ASSERT_EQUAL_HEX16(0, decoded.status);
}

void when_packedTwice_expect_seqIncremented()
{
    pack_and_decode();
    uint16_t first = decoded.seq;

    pack_and_decode();

    TEST_ASSERT_EQUAL_UINT16((uint16_t)(first + 1), decoded.seq);
}

#pragma endregion

#pragma region saturation

void when_valuesOutOfRange_expect_clampedToLimits()
{
    ds.env.temp_c = 1000.0f;      // max 327.67
    ds.env.pressure_pa = 1000.0f; // min 30000
    ds.env.humidity_pct = 200.0f; // max 127.5
    ds.imu.accel[0] = -100.0f;    // min -32.768
    ds.uv.uv_index = -1.0f;       // min 0
    ds.battery.voltage = 10.0f;   // max 4.55

    pack_and_decode();

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 327.67f, decoded.temp_c);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30000.0f, decoded.pressure_pa);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 127.5f, decoded.humidity_pct);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -32.768f, decoded.accel[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, decoded.uv_index);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.55f, decoded.vbatt);
}

void when_valueIsNan_expect_lowestValue()
{
    ds.env.temp_c = NAN;

    pack_and_decode();

    TEST_ASSERT_FLOAT_WITHIN(0.001f, -327.68f, decoded.temp_c);
}

void when_tooManySatsOrFixType_expect_clampedToBitfield()
{
    ds.gps.sats = 40;    // max 31
    ds.gps.fix_type = 9; // max 7

    pack_and_decode();

    TEST_ASSERT_EQUAL_UINT8(31, decoded.sats);
    TEST_ASSERT_EQUAL_UINT8(7, decoded.fix_type);
}

void when_heaterOnLong_expect_saturatedAt255()
{
    ds.gas.heater_enabled = true;
    ds.gas.heater_on_s = 1000;

    pack_and_decode();

    TEST_ASSERT_EQUAL_UINT8(255, decoded.heater_on_s);
}

void when_heaterOff_expect_zeroSeconds()
{
    ds.gas.heater_enabled = false;
    ds.gas.heater_on_s = 100;

    pack_and_decode();

    TEST_ASSERT_EQUAL_UINT8(0, decoded.heater_on_s);
}

#pragma endregion

#pragma region rejected_packets

void when_lengthWrong_expect_badLength()
{
    ds_to_lora(&packet, &ds);
    decoded.seq = 0xBEEF;

    TEST_ASSERT_EQUAL(LORA_DECODE_BAD_LENGTH, lora_decode(&decoded, (const uint8_t *)&packet, sizeof(packet) - 1));
    TEST_ASSERT_EQUAL(LORA_DECODE_BAD_LENGTH, lora_decode(&decoded, (const uint8_t *)&packet, 0));
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, decoded.seq); // out untouched
}

void when_anyByteCorrupted_expect_badCrc()
{
    fill_datastore();
    ds_to_lora(&packet, &ds);

    for (size_t i = 0; i < sizeof(packet); i++)
    {
        uint8_t buf[sizeof(packet)];
        memcpy(buf, &packet, sizeof(packet));
        buf[i] ^= 0x10;

        TEST_ASSERT_EQUAL_MESSAGE(LORA_DECODE_BAD_CRC, lora_decode(&decoded, buf, sizeof(buf)), "byte flipped");
    }
}

void when_syncWrongButCrcOk_expect_badSync()
{
    ds_to_lora(&packet, &ds);
    packet.sync = 0x1234;
    refresh_crc();

    TEST_ASSERT_EQUAL(LORA_DECODE_BAD_SYNC, lora_decode(&decoded, (const uint8_t *)&packet, sizeof(packet)));
}

void when_versionWrongButCrcOk_expect_badVersion()
{
    ds_to_lora(&packet, &ds);
    packet.version = LORA_VERSION + 1;
    refresh_crc();

    TEST_ASSERT_EQUAL(LORA_DECODE_BAD_VERSION, lora_decode(&decoded, (const uint8_t *)&packet, sizeof(packet)));
}

void when_bufferUnaligned_expect_decodedCorrectly()
{
    fill_datastore();
    ds_to_lora(&packet, &ds);

    uint8_t buf[sizeof(packet) + 1];
    memcpy(&buf[1], &packet, sizeof(packet));

    TEST_ASSERT_EQUAL(LORA_DECODE_OK, lora_decode(&decoded, &buf[1], sizeof(packet)));
    TEST_ASSERT_DOUBLE_WITHIN(0.5e-7, 45.4641943, decoded.lat);
}

#pragma endregion

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_packetDecoded_expect_sameValuesWithinResolution);
    RUN_TEST(when_allBlocksValid_expect_allStatusBitsSet);
    RUN_TEST(when_noBlockValid_expect_noStatusBitSet);
    RUN_TEST(when_packedTwice_expect_seqIncremented);
    RUN_TEST(when_valuesOutOfRange_expect_clampedToLimits);
    RUN_TEST(when_valueIsNan_expect_lowestValue);
    RUN_TEST(when_tooManySatsOrFixType_expect_clampedToBitfield);
    RUN_TEST(when_heaterOnLong_expect_saturatedAt255);
    RUN_TEST(when_heaterOff_expect_zeroSeconds);
    RUN_TEST(when_lengthWrong_expect_badLength);
    RUN_TEST(when_anyByteCorrupted_expect_badCrc);
    RUN_TEST(when_syncWrongButCrcOk_expect_badSync);
    RUN_TEST(when_versionWrongButCrcOk_expect_badVersion);
    RUN_TEST(when_bufferUnaligned_expect_decodedCorrectly);
    return UNITY_END();
}
