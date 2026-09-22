#include <stdint.h>
#include <stdlib.h>

#include "config.h"

uint8_t buf[BUF_SIZE];

size_t volatile head = 0;
size_t volatile tail = 0;

size_t get_head_masked();
size_t get_tail_masked();

size_t get_head_masked()
{
    return head % BUF_SIZE;
}

size_t get_tail_masked()
{
    return tail % BUF_SIZE;
}
