#pragma once

#include <stdint.h>


#define CRC32_DEFAULT_START         0xFFFFFFFF


uint32_t crc32(uint8_t* buf, int len, uint32_t crc);
