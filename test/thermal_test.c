// compile: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/thermal_test.c src/utils/thermal_summary.c src/codec/pack_data_sd.c src/codec/crc.c mocks/hal_time.c -lm -o build/thermal_runner

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "unity.h"
#include "types.h"
#include "config.h"
#include "crc.h"
#include "thermal_summary.h"
#include "pack_data_sd.h"

// static: a frame is 3 KB
static thermal_t frame;
static thermal_sd_t packed;

static void fill_frame(float c)
{
    frame.s.t_ms = 1234;
    frame.s.valid = true;
    for (size_t i = 0; i < THERMAL_PIXELS; i++)
        frame.pixels[i] = c;
}

void setUp()
{
    memset(&frame, 0, sizeof(frame));
    memset(&packed, 0, sizeof(packed));
}

void tearDown() {}

#pragma region summary

void when_summarizingFrame_expect_minMaxMeanAndHottestPixel()
{
    fill_frame(20.0f);
    frame.pixels[10] = 10.0f;
    frame.pixels[500] = 80.0f;

    thermal_summary_t summary;
    thermal_summarize(&summary, &frame);

    float expected_mean = (20.0f * (THERMAL_PIXELS - 2) + 10.0f + 80.0f) / THERMAL_PIXELS;

    TEST_ASSERT_TRUE(summary.s.valid);
    TEST_ASSERT_EQUAL_UINT32(1234, summary.s.t_ms);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, summary.min_c);
    TEST_ASSERT_EQUAL_FLOAT(80.0f, summary.max_c);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected_mean, summary.mean_c);
    TEST_ASSERT_EQUAL_UINT16(500, summary.hottest_px);
}

void when_summarizingFrameWithNanPixels_expect_nanPixelsSkipped()
{
    fill_frame(20.0f);
    frame.pixels[0] = NAN;
    frame.pixels[1] = NAN;
    frame.pixels[2] = 30.0f;

    thermal_summary_t summary;
    thermal_summarize(&summary, &frame);

    float expected_mean = (20.0f * (THERMAL_PIXELS - 3) + 30.0f) / (THERMAL_PIXELS - 2);

    TEST_ASSERT_TRUE(summary.s.valid);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, summary.min_c);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, summary.max_c);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected_mean, summary.mean_c);
    TEST_ASSERT_EQUAL_UINT16(2, summary.hottest_px);
}

void when_summarizingFrameAllNan_expect_notValid()
{
    fill_frame(NAN);

    thermal_summary_t summary;
    thermal_summarize(&summary, &frame);

    TEST_ASSERT_FALSE(summary.s.valid);
    TEST_ASSERT_EQUAL_UINT32(1234, summary.s.t_ms);
}

#pragma endregion

#pragma region sd

void when_packingFrame_expect_headerAndCrcCorrect()
{
    fill_frame(25.0f);

    thermal_to_sd(&packed, &frame);

    TEST_ASSERT_EQUAL_HEX16(SYNC_WORD, packed.header.sync);
    TEST_ASSERT_EQUAL_UINT8(DATA_TYPE_THERMAL, packed.header.type);
    TEST_ASSERT_EQUAL_UINT16(sizeof(thermal_packed_t), packed.header.len);
    TEST_ASSERT_EQUAL_UINT32(1234, packed.data.s.t_ms);
    TEST_ASSERT_EQUAL_UINT8(1, packed.data.s.valid);
    // the crc of the whole record, crc included, is 0 if nothing changed
    TEST_ASSERT_EQUAL_UINT16(0, crc16((const uint8_t *)&packed, sizeof(packed)));
}

void when_packingPixels_expect_hundredthsOfDegreeRounded()
{
    fill_frame(0.0f);
    frame.pixels[0] = 21.234f;
    frame.pixels[1] = -12.346f;
    frame.pixels[THERMAL_PIXELS - 1] = 299.999f;

    thermal_to_sd(&packed, &frame);

    TEST_ASSERT_EQUAL_INT16(2123, packed.data.pixels[0]);
    TEST_ASSERT_EQUAL_INT16(-1235, packed.data.pixels[1]);
    TEST_ASSERT_EQUAL_INT16(0, packed.data.pixels[2]);
    TEST_ASSERT_EQUAL_INT16(30000, packed.data.pixels[THERMAL_PIXELS - 1]);
}

void when_packingPixelsOutOfRangeOrNan_expect_clampedOrInvalid()
{
    fill_frame(0.0f);
    frame.pixels[0] = 1000.0f;
    frame.pixels[1] = -1000.0f;
    frame.pixels[2] = NAN;

    thermal_to_sd(&packed, &frame);

    TEST_ASSERT_EQUAL_INT16(INT16_MAX, packed.data.pixels[0]);
    TEST_ASSERT_EQUAL_INT16(-INT16_MAX, packed.data.pixels[1]);
    TEST_ASSERT_EQUAL_INT16(THERMAL_PIXEL_INVALID, packed.data.pixels[2]);
}

#pragma endregion

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_summarizingFrame_expect_minMaxMeanAndHottestPixel);
    RUN_TEST(when_summarizingFrameWithNanPixels_expect_nanPixelsSkipped);
    RUN_TEST(when_summarizingFrameAllNan_expect_notValid);
    RUN_TEST(when_packingFrame_expect_headerAndCrcCorrect);
    RUN_TEST(when_packingPixels_expect_hundredthsOfDegreeRounded);
    RUN_TEST(when_packingPixelsOutOfRangeOrNan_expect_clampedOrInvalid);
    return UNITY_END();
}
