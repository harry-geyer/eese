#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "pinmap.h"
#include "util.h"
#include "systick.h"
#include "uarts.h"
#include "itf.h"
#include "htu21d.h"


#define FLASHING_DELAY_MS        1000


int main(void)
{
    systick_init();

    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_clear(LED_PORT, LED_PIN);

    uarts_init();
    htu21d_init();

    uint32_t prev_now = 0;
    while(1) {
        uint32_t time_passed = since_boot_delta(get_since_boot_ms(), prev_now);
        while(time_passed < FLASHING_DELAY_MS) {
            time_passed = since_boot_delta(get_since_boot_ms(), prev_now);
            itf_iterate();
            htu21d_iterate();
        }

        prev_now = get_since_boot_ms();
        gpio_toggle(LED_PORT, LED_PIN);
    }
    return 0;
}
