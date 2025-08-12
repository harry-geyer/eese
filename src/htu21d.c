#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <libopencm3/stm32/i2c.h>

#include "util.h"
#include "crc.h"
#include "itf.h"
#include "pinmap.h"
#include "systick.h"


#define HTU21D_I2C_ADDR                         0x40
#define HTU21D_DELAY_CLEAR_MS                   90UL
#define HTU21D_DELAY_TEMP_MS                    16UL
#define HTU21D_DELAY_HUMI_MS                    16UL

#define HTU21D_TRANSFER(_w, _wn, _r, _rn)       i2c_transfer7_timeout(I2C_HTU21D_PERIPH, HTU21D_I2C_ADDR, _w, _wn, _r, _rn, I2C_DEFAULT_TIMEOUT_MS)


typedef enum {
    HTU21D_COMMAND_HOLD_TRIG_TEMP_MEAS = 0xE3,
    HTU21D_COMMAND_HOLD_TRIG_HUMI_MEAS = 0xE5,
    HTU21D_COMMAND_TRIG_TEMP_MEAS = 0xF3,
    HTU21D_COMMAND_TRIG_HUMI_MEAS = 0xF5,
    HTU21D_COMMAND_WRITE_USER_REG = 0xE6,
    HTU21D_COMMAND_READ_USER_REG = 0xE7,
    HTU21D_COMMAND_SOFT_RESET = 0xFE,
} _htu21d_command_t;


static bool _htu21d_command(const _htu21d_command_t command);
static bool _htu21d_read(uint16_t* data);
static int32_t _htu21d_conv_temperature(uint16_t s_temp);
static int32_t _htu21d_conv_humidity(uint16_t s_humi);


void htu21d_init(void)
{
    rcc_periph_clock_enable(I2C_HTU21D_RCC);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_set_i2c_clock_hsi(I2C_HTU21D_PERIPH);

    rcc_periph_reset_pulse(I2C_HTU21D_RST);

    rcc_periph_clock_enable(I2C_HTU21D_RCC);
    i2c_peripheral_enable(I2C_HTU21D_PERIPH);
    gpio_mode_setup(I2C_HTU21D_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, I2C_HTU21D_PINS);
    gpio_set_af(I2C_HTU21D_GPIO_PORT, I2C_HTU21D_AF, I2C_HTU21D_PINS);
    i2c_peripheral_disable(I2C_HTU21D_PERIPH);
    //configure ANFOFF DNF[3:0] in CR1
    i2c_enable_analog_filter(I2C_HTU21D_PERIPH);
    i2c_set_digital_filter(I2C_HTU21D_PERIPH, 0);
    /* HSI is at 8Mhz */
    i2c_set_speed(I2C_HTU21D_PERIPH, i2c_speed_sm_100k, 8);
    //configure No-Stretch CR1 (only relevant in slave mode)
    i2c_enable_stretching(I2C_HTU21D_PERIPH);
    //addressing mode
    i2c_set_7bit_addr_mode(I2C_HTU21D_PERIPH);
    i2c_peripheral_enable(I2C_HTU21D_PERIPH);

    _htu21d_command(HTU21D_COMMAND_SOFT_RESET);
}


void htu21d_iterate(void)
{
    static itf_measurements_t _measurements = {0};
    static uint32_t _last_measurement_time = 0UL;
    static uint32_t _delay_ms = HTU21D_DELAY_CLEAR_MS;
    static enum {
        HTU21D_STATE_CLEAR,
        HTU21D_STATE_READ_TEMP,
        HTU21D_STATE_READ_HUMI,
    } _state = HTU21D_STATE_CLEAR;
    if (since_boot_delta(get_since_boot_ms(), _last_measurement_time) <= _delay_ms) {
        /* not enough time has passed */
        return;
    }
    switch (_state) {
        case HTU21D_STATE_CLEAR:
            if (_htu21d_command(HTU21D_COMMAND_HOLD_TRIG_TEMP_MEAS)) {
                /* succeeded sending command */
                _state = HTU21D_STATE_READ_TEMP;
                _delay_ms = HTU21D_DELAY_TEMP_MS;
            }
            break;
        case HTU21D_STATE_READ_TEMP: {
            uint16_t temp16 = 0;
            if (_htu21d_read(&temp16) && _htu21d_command(HTU21D_COMMAND_HOLD_TRIG_HUMI_MEAS)) {
                /* succeeded reading temperature */
                _state = HTU21D_STATE_READ_HUMI;
                _measurements.temperature = _htu21d_conv_temperature(temp16);
                _delay_ms = HTU21D_DELAY_HUMI_MS;
            } else {
                _state = HTU21D_STATE_CLEAR;
                _delay_ms = HTU21D_DELAY_CLEAR_MS;
            }
            break;
        }
        case HTU21D_STATE_READ_HUMI: {
            uint16_t humi16 = 0;
            if (_htu21d_read(&humi16)) {
                /* succeeded reading humidity - can only reach here if
                 * valid temperature is given so can construct a packet
                 * with both */
                _measurements.relative_humdity = _htu21d_conv_humidity(humi16);
                itf_send_measurements(&_measurements);
            }
            _state = HTU21D_STATE_CLEAR;
            _delay_ms = HTU21D_DELAY_CLEAR_MS;
            break;
        }
        default:
            _state = HTU21D_STATE_CLEAR;
            _delay_ms = HTU21D_DELAY_CLEAR_MS;
            break;
    }
    _last_measurement_time = get_since_boot_ms();
}


static bool _htu21d_command(const _htu21d_command_t command)
{
    uint8_t command8 = command;
    return HTU21D_TRANSFER(&command8, 1, NULL, 0);
}


static bool _htu21d_read(uint16_t* data)
{
    uint8_t data_arr[3] = {0};
    if (!HTU21D_TRANSFER(NULL, 0, data_arr, 3)) {
        return false;
    }
    if (crc8(data_arr, 3)) {
        /* Invalid CRC8 */
        return false;
    }
    *data = (data_arr[0] << 8) | data_arr[1];
    return true;
}


static int32_t _htu21d_conv_temperature(uint16_t s_temp)
{
    return 17572L * s_temp / (1 << 16) - 4685L;
}


static int32_t _htu21d_conv_humidity(uint16_t s_humi)
{
    return 12500L * s_humi / (1 << 16) - 600L;
}
