#pragma once

#include <stdint.h>


uint32_t uart_rings_in_add(char* msg, uint32_t len);
uint32_t uart_rings_out_add(char* msg, uint32_t len);
uint32_t uart_rings_in_drain(char* msg, uint32_t len);
uint32_t uart_rings_out_drain(char* msg, uint32_t len);
