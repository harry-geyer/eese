#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/i2c.h>

#include "systick.h"


uint32_t since_boot_delta(uint32_t newer, uint32_t older)
{
    if (newer < older) {
        return (0xFFFFFFFF - older) + newer;
    } else {
        return newer - older;
    }
}


bool i2c_transfer7_timeout(uint32_t i2c, uint8_t addr, const uint8_t* w, size_t wn, uint8_t* r, size_t rn, uint32_t timeout_ms)
{
    uint32_t start_time = get_since_boot_ms();
    if (wn) {
        i2c_set_7bit_address(i2c, addr);
        i2c_set_write_transfer_dir(i2c);
        i2c_set_bytes_to_transfer(i2c, wn);
        if (rn) {
            i2c_disable_autoend(i2c);
        } else {
            i2c_enable_autoend(i2c);
        }
        i2c_send_start(i2c);

        while (wn--) {
            bool wait = true;
            while (wait) {
                if (i2c_transmit_int_status(i2c)) {
                    wait = false;
                }
                while (i2c_nack(i2c)) {
                    if (since_boot_delta(get_since_boot_ms(), start_time) > timeout_ms) {
                        return false;
                    }
                }
            }
            i2c_send_data(i2c, *w++);
        }
        /* not entirely sure this is really necessary.
         * RM implies it will stall until it can write out the later bits
         */
        if (rn) {
            while (!i2c_transfer_complete(i2c)) {
                if (since_boot_delta(get_since_boot_ms(), start_time) > timeout_ms) {
                    return false;
                }
            }
        }
    }

    if (rn) {
        /* Setting transfer properties */
        i2c_set_7bit_address(i2c, addr);
        i2c_set_read_transfer_dir(i2c);
        i2c_set_bytes_to_transfer(i2c, rn);
        /* start transfer */
        i2c_send_start(i2c);
        /* important to do it afterwards to do a proper repeated start! */
        i2c_enable_autoend(i2c);

        for (size_t i = 0; i < rn; i++) {
            while (i2c_received_data(i2c) == 0) {
                if (since_boot_delta(get_since_boot_ms(), start_time) > timeout_ms) {
                    return false;
                }
            }
            r[i] = i2c_get_data(i2c);
        }
    }
    return true;
}
