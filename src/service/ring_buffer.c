#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "types.h"
#include "datastore.h"
#include "hal_time.h"

uint8_t buf[BUF_SIZE];

size_t volatile head = 0;
size_t volatile tail = 0;

// ONLY for testing purposes
void buf_reset()
{
    head = 0;
    tail = 0;
}

void buf_write(void *data, size_t len)
{
    if (len > BUF_SIZE - (head - tail)) // Not enough free space in the buffer
    {
        ds_write_increment_records_dropped(hal_millis());
        return;
    }

    uint8_t *bytes = (uint8_t *)data;

    size_t head_masked = head % BUF_SIZE;

    size_t first_chunk_max_size = BUF_SIZE - head_masked;

    if (first_chunk_max_size >= len)
    {
        memcpy(&buf[head_masked], bytes, len);
    }
    else
    {
        memcpy(&buf[head_masked], bytes, first_chunk_max_size);
        memcpy(buf, bytes + first_chunk_max_size, len - first_chunk_max_size);
    }

    __sync_synchronize();
    head += len;
}

// Having the function like this might mean having to keep in RAM another array
// of the same size of the buffer in order to ensure that the copy is successful.
// It might be better to have a function that directly saves to SD without needing this thing,
// but I am not sure it's viable.
// Else this would be a great usecase for malloc, but in this project it's better not to use it.
void buf_read(uint8_t *out)
{
    size_t current_head = head; // so if head changes while copying strange errors don't occur (conservative approach)
    size_t tail_masked = tail % BUF_SIZE;

    size_t first_chunk_max_size = BUF_SIZE - tail_masked;
    if (first_chunk_max_size >= current_head - tail)
    {
        memcpy(out, &buf[tail_masked], current_head - tail);
    }
    else
    {
        memcpy(out, &buf[tail_masked], first_chunk_max_size);
        memcpy(out + first_chunk_max_size, buf, current_head - tail - first_chunk_max_size);
    }

    __sync_synchronize();
    tail += current_head - tail;
}
