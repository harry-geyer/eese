#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>


static uint32_t _systick_since_boot_ms = 0;


void __attribute__((interrupt)) sys_tick_handler(void)
{
    _systick_since_boot_ms++;
}


void systick_init(void)
{
    systick_set_frequency(1000, rcc_ahb_frequency);
    systick_counter_enable();
    systick_interrupt_enable();
}


uint32_t get_since_boot_ms(void)
{
    return _systick_since_boot_ms;
}
