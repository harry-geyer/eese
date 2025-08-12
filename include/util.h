#pragma once

#include <stdint.h>
#include <stddef.h>


#define PORT_TO_RCC(_port_)   (RCC_GPIOA + ((_port_ - GPIO_PORT_A_BASE) / 0x400))


#define I2C_DEFAULT_TIMEOUT_MS           10


uint32_t since_boot_delta(uint32_t newer, uint32_t older);
bool i2c_transfer7_timeout(uint32_t i2c, uint8_t addr, const uint8_t* w, size_t wn, uint8_t* r, size_t rn, uint32_t timeout_ms);
