// compile: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/crc_test.c src/codec/crc.c -o build/crc_runner

#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "crc.h"
#include "crc16_vectors.h"

void setUp() {}

void tearDown() {}

void when_shortWordCrcCalculated_expect_correctCrc()
{
    uint8_t bytes[9] = {
        0x31,
        0x32,
        0x33,
        0x34,
        0x35,
        0x36,
        0x37,
        0x38,
        0x39,
    };

    uint16_t crc = crc16(bytes, 9);

    printf("Expected crc: %u\n", 0x4B37);
    printf("Actual crc: %u\n", crc);

    TEST_ASSERT_EQUAL_UINT16(0x4B37, crc);
}

void when_crcDecodedCorrectly_expect_zero()
{
    uint8_t bytes[11] = {
        0x31,
        0x32,
        0x33,
        0x34,
        0x35,
        0x36,
        0x37,
        0x38,
        0x39,
        0,
        0,
    };

    uint16_t crc = 0x4B37;

    // Little endian copy
    memcpy(&bytes[9], (uint8_t *)&crc, 2);

    uint16_t check_crc = crc16(bytes, 11);

    printf("Check crc: %u\n", check_crc);
    TEST_ASSERT_EQUAL_UINT16(0, check_crc);
}

void when_manyDifferentLengthCrcCalculated_expect_CorrectCrc()
{
    size_t failures = 0;

    for (size_t i = 0; i < CRC16_NUM_VECTORS; i++)
    {
        const crc16_vector_t *v = &crc16_vectors[i];
        uint16_t got = crc16(v->data, v->len);
        if (got != v->expected)
        {
            printf("FAIL [%zu] %s: expected 0x%04X, got 0x%04X\n",
                   i, v->name, v->expected, got);
            failures++;
        }
    }

    printf("Number of failures: %lu\n", failures);
    TEST_ASSERT_EQUAL(0, failures);
}

void when_manyDifferentLengthCrcDecoded_expect_zero()
{
    size_t failures = 0;

    for (size_t i = 0; i < CRC16_NUM_VECTORS; i++)
    {
        const crc16_vector_t *v = &crc16_vectors[i];
        uint8_t frame[1024 + 2];
        if (v->len > 1024)
            continue;
        memcpy(frame, v->data, v->len);
        uint16_t crc = crc16(frame, v->len);
        memcpy(&frame[v->len], &crc, 2);
        if (crc16(frame, v->len + 2) != 0)
        {
            printf("FAIL round-trip [%zu] %s\n", i, v->name);
            failures++;
        }
    }

    printf("Number of failures: %lu\n", failures);
    TEST_ASSERT_EQUAL(0, failures);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_shortWordCrcCalculated_expect_correctCrc);
    RUN_TEST(when_crcDecodedCorrectly_expect_zero);
    RUN_TEST(when_manyDifferentLengthCrcCalculated_expect_CorrectCrc);
    RUN_TEST(when_manyDifferentLengthCrcDecoded_expect_zero);
    return UNITY_END();
}
