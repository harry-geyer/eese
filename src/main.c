#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "pinmap.h"
#include "uarts.h"


int main(void)
{
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_clear(LED_PORT, LED_PIN);

    uarts_init();

    while(1) {
        unsigned limit = rcc_ahb_frequency / 1000UL;
        for(unsigned n = 0; n < limit; n++) {
            __asm__("NOP");
        }

        gpio_toggle(LED_PORT, LED_PIN);
    }
    return 0;
}
