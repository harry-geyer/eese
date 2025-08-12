#pragma once

#include <stdint.h>


typedef struct {
    volatile uint8_t* buf;
    uint32_t size;
    volatile uint32_t r_pos;
    volatile uint32_t w_pos;
} ring_buf_t;


#define RING_BUF_INIT(_buf, _size)                                      \
{                                                                       \
    .buf = _buf,                                                        \
    .size = _size,                                                      \
    .r_pos = _size - 1,                                                 \
    .w_pos = 0                                                          \
}


uint32_t ring_buf_write(ring_buf_t* ring, uint8_t* data, uint32_t count);
uint32_t ring_buf_peek(ring_buf_t* ring, uint8_t* data, uint32_t count);
uint32_t ring_buf_read(ring_buf_t* ring, uint8_t* data, uint32_t count);
uint32_t ring_buf_read_until(ring_buf_t* ring, uint8_t* data, uint32_t count, uint8_t until);
