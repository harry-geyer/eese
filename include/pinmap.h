#pragma once

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>

#define LED_PORT                GPIOA
#define LED_PIN                 GPIO5

#define UART_ITF_GPIO_PORT      GPIOA
#define UART_ITF_PINS           (GPIO2 | GPIO3)
#define UART_ITF_AF             GPIO_AF1
#define UART_ITF_UART           USART2
#define UART_ITF_CLK            RCC_USART2
#define UART_ITF_IRQ            NVIC_USART2_IRQ
#define UART_ITF_DMA_CHAN       DMA_CHANNEL4
#define UART_ITF_DMA_IRQ        NVIC_DMA1_CHANNEL4_7_DMA2_CHANNEL3_5_IRQ
