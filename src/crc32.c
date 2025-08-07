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
