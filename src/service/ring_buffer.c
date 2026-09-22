#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "types.h"

uint8_t buf[BUF_SIZE];

size_t volatile head = 0;
size_t volatile tail = 0;

void buf_write(void *data, size_t len)
{
    uint8_t *bytes = (const uint8_t *)data;

    size_t head_masked = head % BUF_SIZE;

    size_t first_chunk_max_size = BUF_SIZE - head_masked;

    if (first_chunk_max_size >= len)
    {
        memcpy(&buf[head_masked], bytes, len);
    }
    else
    {
        memcpy(&buf[head_masked], bytes, first_chunk_max_size);
        memcpy(&buf[(head + first_chunk_max_size) % BUF_SIZE], bytes, len - first_chunk_max_size);
    }

    __sync_synchronize();
    head += len;
}

void buf_read(uint8_t *out)
{
    size_t current_head = head; // so if head changes while copying strange errors don't occur (conservative approach)
    size_t head_masked = current_head % BUF_SIZE;
    size_t tail_masked = tail % BUF_SIZE;

    size_t first_chunk_max_size = BUF_SIZE - tail_masked;
    if (first_chunk_max_size >= current_head - tail)
    {
        memcpy(out, buf, current_head - tail);
    }
    else
    {
        memcpy(out, buf, first_chunk_max_size);
        memcpy(out[first_chunk_max_size - 1], buf, current_head - tail - first_chunk_max_size);
    }

    __sync_synchronize();
    tail += current_head - tail;
}
