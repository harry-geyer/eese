#include <stdint.h>

#include "ring_buf.h"


#define UART_RING_IN_BUF_SIZE               128
#define UART_RING_OUT_BUF_SIZE              256


static volatile uint8_t _uart_ring_in_buf[UART_RING_IN_BUF_SIZE];
static volatile uint8_t _uart_ring_out_buf[UART_RING_OUT_BUF_SIZE];

static ring_buf_t _uart_ring_in = RING_BUF_INIT(_uart_ring_in_buf, UART_RING_IN_BUF_SIZE);
static ring_buf_t _uart_ring_out = RING_BUF_INIT(_uart_ring_out_buf, UART_RING_OUT_BUF_SIZE);


uint32_t uart_rings_in_add(char* msg, uint32_t len)
{
    return ring_buf_write(&_uart_ring_in, (uint8_t*)msg, len);
}


uint32_t uart_rings_out_add(char* msg, uint32_t len)
{
    return ring_buf_write(&_uart_ring_out, (uint8_t*)msg, len);
}


uint32_t uart_rings_in_drain(char* msg, uint32_t len)
{
    return ring_buf_read_until(&_uart_ring_in, (uint8_t*)msg, len, (uint8_t)'\n');
}


uint32_t uart_rings_out_drain(char* msg, uint32_t len)
{
    return ring_buf_read(&_uart_ring_out, (uint8_t*)msg, len);
}
