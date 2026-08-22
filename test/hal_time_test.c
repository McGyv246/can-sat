// Questo è solo un test per vedere se esegue senza crashare (DA RIMUOVERE QUANDO SI USA MAKE)
// compilazione: gcc -I mocks -Wall test/hal_time_test.c mocks/hal_time.c -o build/hal_time_runner

#include "hal_time.h"
#include <stdio.h>

void setUp() {}

void tearDown() {}

int main()
{
    printf("%u\n", hal_millis());
    printf("%lu\n", hal_micros());
}
