// compile: gcc -fsanitize=address -I mocks -I test/unity -Wall test/unity/unity.c test/hal_time_test.c mocks/hal_time.c -o build/hal_time_runner

#include "hal_time.h"
#include "unity.h"
#include <stdio.h>
#include <stdint.h>

void setUp() {}

void tearDown() {}

void when_gettingTimeMillis_expect_sequentiality()
{
    uint32_t time1 = hal_millis();
    printf("%u\n", time1);

    // Crea del delay tra le due misure
    for (size_t i = 0; i < 1000000; i++)
        ;

    uint32_t time2 = hal_millis();
    printf("%u\n", time2);

    TEST_ASSERT_GREATER_OR_EQUAL(time1, time2);
}

void when_gettingTimeMicros_expect_sequentiality()
{
    uint64_t time1 = hal_micros();
    printf("%lu\n", time1);

    // Crea del delay tra le due misure
    for (size_t i = 0; i < 1000000; i++)
        ;

    uint64_t time2 = hal_micros();
    printf("%lu\n", time2);

    TEST_ASSERT_GREATER_OR_EQUAL(time1, time2);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_gettingTimeMillis_expect_sequentiality);
    RUN_TEST(when_gettingTimeMicros_expect_sequentiality);
    return UNITY_END();
}
