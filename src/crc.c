#include <stdint.h>


uint32_t crc32(uint8_t* buf, int len, uint32_t crc)
{
    int i, j;
    uint32_t b, msk;
    i = 0;
    while (i < len) {
        b = buf[i];
        crc = crc ^ b;
        for (j = 7; j >= 0; j--) {
            msk = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & msk);
        }
        i = i + 1;
    }
    return crc;
}


uint8_t crc8(uint8_t* buf, int len)
{
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = buf[i];
        crc ^= byte;
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc <<= 1;
                crc ^= 0x131;
            }
            else crc <<= 1;
        }
    }
    return crc;
}
