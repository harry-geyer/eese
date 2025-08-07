#include <stdint.h>

#include <libopencm3/cm3/scb.h>


#define SYSTEM_FW_ADDR          0x800000


void system_reset(void)
{
    SCB_VTOR = SYSTEM_FW_ADDR & 0xFFFF;
    /* Initialise master stack pointer. */
    asm volatile("msr msp, %0"::"g"(*(volatile uint32_t *)SYSTEM_FW_ADDR));
    /* Jump to application. */
    (*(void (**)())(SYSTEM_FW_ADDR + 4))();
}
