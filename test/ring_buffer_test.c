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

void setUp() {}

void tearDown() {}

void when_insertingOneStructInBuffer_expect_retrievingCorrectData()
{
    size_t record_size = sizeof(stats_packed_t) + sizeof(sd_header_t) + sizeof(uint16_t);
    buf_write(&test_stats, record_size);

    printf("Record size: %lu\n", record_size);

    print_stats_sd(&test_stats);

    printf("\n");

    uint8_t out[record_size];
    stats_sd_t parsed_stats;
    buf_read(out);
    memcpy(&parsed_stats, out, record_size);

    print_stats_sd(&parsed_stats);
}

void when_insertingMultipleStructsInBuffer_expect_retrievingCorrectData()
{
}

void when_insertingAtWrapInBufferAndReading_expect_dataToBeRetrievedCorrectly()
{
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
    RUN_TEST(when_insertingAtWrapInBufferAndReading_expect_dataToBeRetrievedCorrectly);
    RUN_TEST(when_insertingInFullBuffer_expect_frameToBeDroppedAndDatastoreUpdated);
    RUN_TEST(when_readingFromEmptyBuffer_expect_emptyPointerAndNoCrash);
    return UNITY_END();
}