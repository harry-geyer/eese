#pragma once

#include <stdint.h>


#define PORT_TO_RCC(_port_)   (RCC_GPIOA + ((_port_ - GPIO_PORT_A_BASE) / 0x400))


uint32_t since_boot_delta(uint32_t newer, uint32_t older);
