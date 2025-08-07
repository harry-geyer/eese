#include <stdint.h>


uint32_t since_boot_delta(uint32_t newer, uint32_t older)
{
    if (newer < older) {
        return (0xFFFFFFFF - older) + newer;
    } else {
        return newer - older;
    }
}
