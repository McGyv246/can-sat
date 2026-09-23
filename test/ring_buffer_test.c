// compile: gcc -fsanitize=address -DBUF_SIZE=100 -I include -I mocks -I test/unity -Wall test/unity/unity.c test/ring_buffer_test.c mocks/hal_time.c src/service/datastore.c src/service/ring_buffer.c -o build/ring_buffer_runner

#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "config.h"
#include "ring_buffer.h"

stats_sd_t test_stats = {
    .header = {
        .len = sizeof(stats_packed_t),
        .type = STATS,
    },
    .data = {
        .t_ms = 1000,
        .state = 1,
        .alt_peak_env = 100,
        .alt_peak_gps = 101,
        .records_dropped = 0,
    },
    .crc = 0,
};

gas_sd_t test_gas = {
    .header = {
        .len = sizeof(gas_packed_t),
        .type = GAS,
    },
    .data = {
        .s = {
            2000,
            1,
        },
        .raw = 100,
        .heater_on_s = 500,
        .heater_enabled = 1,
    },
    .crc = 0,
};

void print_sd_header(sd_header_t *data)
{
    printf("sync: %u\n", data->sync);
    printf("type: %u\n", data->type);
    printf("len: %u\n", data->len);
    printf("header t_ms: %u\n", data->t_ms);
}

void print_stats_sd(stats_sd_t *data)
{
    print_sd_header(&(data->header));

    printf("t_ms: %u\n", data->data.t_ms);
    printf("state: %d\n", data->data.state);
    printf("alt_peak_env: %f\n", data->data.alt_peak_env);
    printf("alt_peak_gps: %f\n", data->data.alt_peak_gps);
    printf("records_dropped: %u\n", data->data.records_dropped);

    printf("crc: %u\n", data->crc);
}

void print_gas_sd(gas_sd_t *data)
{
    print_sd_header(&(data->header));

    printf("s.t_ms: %u\n", data->data.s.t_ms);
    printf("s.valid: %u\n", data->data.s.valid);
    printf("raw: %u\n", data->data.raw);
    printf("heater_on_s: %u\n", data->data.heater_on_s);
    printf("heater_enabled: %u\n", data->data.heater_enabled);

    printf("crc: %u\n", data->crc);
}

void assert_equal_sd_header(sd_header_t *expected, sd_header_t *actual)
{
    TEST_ASSERT_EQUAL_UINT32(expected->sync, actual->sync);
    TEST_ASSERT_EQUAL_UINT32(expected->type, actual->type);
    TEST_ASSERT_EQUAL_UINT32(expected->len, actual->len);
    TEST_ASSERT_EQUAL_UINT32(expected->t_ms, actual->t_ms);
}

void assert_equal_stats_sd(stats_sd_t *expected, stats_sd_t *actual)
{
    assert_equal_sd_header(&(expected->header), &(actual->header));

    TEST_ASSERT_EQUAL_UINT32(expected->data.t_ms, actual->data.t_ms);
    TEST_ASSERT_EQUAL_INT(expected->data.state, actual->data.state);
    TEST_ASSERT_EQUAL_FLOAT(expected->data.alt_peak_env, actual->data.alt_peak_env);
    TEST_ASSERT_EQUAL_FLOAT(expected->data.alt_peak_gps, actual->data.alt_peak_gps);
    TEST_ASSERT_EQUAL_UINT32(expected->data.records_dropped, actual->data.records_dropped);

    TEST_ASSERT_EQUAL_UINT32(expected->crc, actual->crc);
}

void assert_equal_gas_sd(gas_sd_t *expected, gas_sd_t *actual)
{
    assert_equal_sd_header(&(expected->header), &(actual->header));

    TEST_ASSERT_EQUAL_UINT32(expected->data.s.t_ms, actual->data.s.t_ms);
    TEST_ASSERT_EQUAL_UINT32(expected->data.s.valid, actual->data.s.valid);
    TEST_ASSERT_EQUAL_UINT32(expected->data.raw, actual->data.raw);
    TEST_ASSERT_EQUAL_UINT32(expected->data.heater_on_s, actual->data.heater_on_s);
    TEST_ASSERT_EQUAL_UINT32(expected->data.heater_enabled, actual->data.heater_enabled);

    TEST_ASSERT_EQUAL_UINT32(expected->crc, actual->crc);
}

void setUp() {}

void tearDown()
{
    buf_reset();
}

void when_insertingOneStructInBuffer_expect_retrievingCorrectData()
{
    size_t record_size = sizeof(stats_sd_t);
    buf_write(&test_stats, record_size);

    printf("Record size: %lu\n", record_size);

    print_stats_sd(&test_stats);

    printf("\n");

    uint8_t stats_bytes[record_size];
    stats_sd_t stats_parsed;
    buf_read(stats_bytes);
    memcpy(&stats_parsed, stats_bytes, record_size);

    print_stats_sd(&stats_parsed);

    assert_equal_stats_sd(&test_stats, &stats_parsed);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t *)&test_stats, stats_bytes, record_size);
}

void when_insertingMultipleStructsInBuffer_expect_retrievingCorrectData()
{
    buf_write(&test_stats, sizeof(stats_sd_t));
    buf_write(&test_gas, sizeof(gas_sd_t));

    stats_sd_t changed_stats = test_stats;
    changed_stats.data.state = 5;
    buf_write(&changed_stats, sizeof(stats_sd_t));

    uint8_t bytes_data[BUF_SIZE];
    buf_read(bytes_data);

    stats_sd_t stats_parsed_1, stats_parsed_2;
    gas_sd_t gas_parsed;

    memcpy(&stats_parsed_1, bytes_data, sizeof(stats_sd_t));
    memcpy(&gas_parsed, bytes_data + sizeof(stats_sd_t), sizeof(gas_sd_t));
    memcpy(&stats_parsed_2, bytes_data + sizeof(stats_sd_t) + sizeof(gas_sd_t), sizeof(stats_sd_t));

    print_stats_sd(&stats_parsed_1);
    printf("\n");
    print_gas_sd(&gas_parsed);
    printf("\n");
    print_stats_sd(&stats_parsed_2);
    printf("\n");

    assert_equal_stats_sd(&test_stats, &stats_parsed_1);
    assert_equal_gas_sd(&test_gas, &gas_parsed);
    assert_equal_stats_sd(&changed_stats, &stats_parsed_2);
}

void when_insertingMultipleStructsInBufferAndReadingInTwoTimes_expect_retrievingCorrectData()
{
    buf_write(&test_stats, sizeof(stats_sd_t));
    buf_write(&test_gas, sizeof(gas_sd_t));

    uint8_t bytes_data[BUF_SIZE];
    buf_read(bytes_data);

    stats_sd_t changed_stats = test_stats;
    changed_stats.data.state = 15;
    buf_write(&changed_stats, sizeof(stats_sd_t));

    buf_read(bytes_data);

    stats_sd_t stats_parsed;
    memcpy(&stats_parsed, bytes_data, sizeof(stats_sd_t));

    print_stats_sd(&stats_parsed);
    printf("\n");

    assert_equal_stats_sd(&changed_stats, &stats_parsed);
}

void when_insertingAtWrapInBufferAndReading_expect_dataToBeRetrievedCorrectly()
{
    buf_write(&test_stats, sizeof(stats_sd_t));
    buf_write(&test_stats, sizeof(stats_sd_t));
    buf_write(&test_gas, sizeof(gas_sd_t));
    buf_write(&test_gas, sizeof(gas_sd_t));

    uint8_t bytes_data[BUF_SIZE];
    buf_read(bytes_data);

    // Now buffer should be empty (head and tail equal)
    // but only 10 slots should be available before wrapping
    // So the next record should wrap

    stats_sd_t changed_stats = test_stats;
    changed_stats.data.state = 25;
    buf_write(&changed_stats, sizeof(stats_sd_t));

    buf_read(bytes_data);

    stats_sd_t stats_parsed;
    memcpy(&stats_parsed, bytes_data, sizeof(stats_sd_t));

    print_stats_sd(&stats_parsed);
    printf("\n");

    assert_equal_stats_sd(&changed_stats, &stats_parsed);
}

void when_insertingInFullBuffer_expect_frameToBeDroppedAndDatastoreUpdated()
{
}

void when_readingFromEmptyBuffer_expect_emptyPointerAndNoCrash()
{
}

int main()
{
    UNITY_BEGIN();
    printf("Buf Size (in test): %d\n", BUF_SIZE);
    RUN_TEST(when_insertingOneStructInBuffer_expect_retrievingCorrectData);
    RUN_TEST(when_insertingMultipleStructsInBuffer_expect_retrievingCorrectData);
    RUN_TEST(when_insertingMultipleStructsInBufferAndReadingInTwoTimes_expect_retrievingCorrectData);
    RUN_TEST(when_insertingAtWrapInBufferAndReading_expect_dataToBeRetrievedCorrectly);
    RUN_TEST(when_insertingInFullBuffer_expect_frameToBeDroppedAndDatastoreUpdated);
    RUN_TEST(when_readingFromEmptyBuffer_expect_emptyPointerAndNoCrash);
    return UNITY_END();
}