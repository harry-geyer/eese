#pragma once

#include <stdint.h>


uint32_t uart_rings_in_add(uint8_t* packet, uint32_t len);
uint32_t uart_rings_out_add(uint8_t* packet, uint32_t len);
uint32_t uart_rings_in_drain(uint8_t* packet, uint32_t len);
uint32_t uart_rings_out_drain(uint8_t* packet, uint32_t len);
