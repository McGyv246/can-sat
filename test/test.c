#include "unity.h"

void setUp() {}

void tearDown() {}

void test_sum(void)
{
    TEST_ASSERT_EQUAL_INT(5, 3 + 2);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sum);
    return UNITY_END();
}