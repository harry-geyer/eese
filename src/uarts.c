#include <stdint.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/nvic.h>

#include "pinmap.h"
#include "util.h"
#include "uart_rings.h"


#define UART_ITF_BAUD           115200
#define UART_ITF_DATA_BITS      8
#define UART_ITF_STOP_BITS      UARTS_STOP_BITS_1
#define UART_ITF_PARITY         UART_PARITY_NONE
#define UART_ITF_FLOWCONTROL    USART_FLOWCONTROL_NONE


typedef enum {
    UARTS_STOP_BITS_1 = 0,
    UARTS_STOP_BITS_1_5 = 1,
    UARTS_STOP_BITS_2 = 2,
} uarts_stop_bits_t;


typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD = 1,
    UART_PARITY_EVEN = 2,
} uarts_parity_t;


static bool _uarts_getc(uint32_t uart, char* c);


int uarts_init(void)
{
    rcc_periph_clock_enable(PORT_TO_RCC(UART_ITF_GPIO_PORT));
    rcc_periph_clock_enable(UART_ITF_CLK);

    gpio_mode_setup(UART_ITF_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART_ITF_PINS);
    gpio_set_af(UART_ITF_GPIO_PORT, UART_ITF_AF, UART_ITF_PINS);

    usart_set_mode(UART_ITF_UART, USART_MODE_TX_RX);
    usart_set_flow_control(UART_ITF_UART, UART_ITF_FLOWCONTROL);

    usart_set_baudrate(UART_ITF_UART, UART_ITF_BAUD);
    usart_set_databits(UART_ITF_UART, UART_ITF_DATA_BITS);
    usart_set_stopbits(UART_ITF_UART, UART_ITF_STOP_BITS);
    usart_set_parity(UART_ITF_UART, UART_ITF_PARITY);

    nvic_enable_irq(UART_ITF_IRQ);
    usart_enable(UART_ITF_UART);
    usart_enable_rx_interrupt(UART_ITF_UART);

    nvic_enable_irq(UART_ITF_DMA_IRQ);
    return 0;
}


void __attribute__((interrupt)) usart3_4_isr(void)
{
    char c = 0;
    if (!_uarts_getc(UART_ITF_UART, &c)) {
        return;
    }
    uart_rings_in_add(&c, 1);
}


void __attribute__((interrupt)) dma1_channel4_7_dma2_channel3_5_isr(void)
{
    if (!(DMA1_ISR & DMA_ISR_TCIF(UART_ITF_DMA_CHAN))) {
        return;
    }
    DMA1_IFCR |= DMA_IFCR_CTCIF(UART_ITF_DMA_CHAN);
    dma_disable_transfer_complete_interrupt(DMA1, UART_ITF_DMA_CHAN);
    usart_disable_tx_dma(UART_ITF_DMA_CHAN);
    dma_disable_channel(DMA1, UART_ITF_DMA_CHAN);
}


static bool _uarts_getc(uint32_t uart, char* c)
{
    uint32_t flags = USART_ISR(uart);
    if (!(flags & USART_ISR_RXNE)) {
        USART_ICR(uart) = flags;
        return false;
    }
    *c = usart_recv(uart);
    return true;
}
