#include <stdint.h>
#include <stddef.h>

#include "ring_buf.h"


static uint32_t _ring_buf_peek(ring_buf_t* ring, uint8_t* data, uint32_t count, volatile uint32_t* end_pos);


uint32_t ring_buf_write(ring_buf_t* ring, uint8_t* data, uint32_t count)
{
    uint32_t i = 0;
    for (; i < count; i++) {
        uint32_t new_pos = (ring->w_pos + 1) % ring->size;
        if (new_pos == ring->r_pos) {
            /* No more space */
            break;
        }
        ring->buf[new_pos] = data[i];
        ring->w_pos = new_pos;
    }
    return i;
}


uint32_t ring_buf_peek(ring_buf_t* ring, uint8_t* data, uint32_t count)
{
    return _ring_buf_peek(ring, data, count, NULL);
}


uint32_t ring_buf_read(ring_buf_t* ring, uint8_t* data, uint32_t count)
{
    return _ring_buf_peek(ring, data, count, &ring->r_pos);
}


uint32_t ring_buf_read_until(ring_buf_t* ring, uint8_t* data, uint32_t count, uint8_t until)
{
    uint32_t i = 0;
    for (; i < count; i++) {
        uint32_t new_pos = (ring->r_pos + 1) % ring->size;
        if (new_pos == ring->w_pos) {
            /* No more space */
            break;
        }
        uint8_t c = ring->buf[new_pos];
        data[i] = c;
        ring->r_pos = new_pos;
        if (c == until) {
            break;
        }
    }
    return i;
}


static uint32_t _ring_buf_peek(ring_buf_t* ring, uint8_t* data, uint32_t count, volatile uint32_t* end_pos)
{
    uint32_t r_pos = ring->r_pos;
    uint32_t i = 0;
    for (; i < count; i++) {
        uint32_t new_pos = (r_pos + 1) % ring->size;
        if (new_pos == ring->w_pos) {
            /* No more space */
            break;
        }
        data[i] = ring->buf[new_pos];
        r_pos = new_pos;
    }
    if (end_pos) {
        *end_pos = r_pos;
    }
    return i;
}
