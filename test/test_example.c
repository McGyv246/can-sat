// Questo è un test di esempio.
// Comando compilazione: gcc -I test/unity -Wall test/test_example.c test/unity/unity.c -o build/example_runner
// Nota che questo comando andrà modificato quando i test utilizzeranno effettivamente file in src/.
// Comando per eseguire il test: ./build/example_runner

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