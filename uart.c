#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include <lpc11xx/LPC11xx.h>

void uart_init(void) {
    /* Configure P1.6 for RXD and P1.7 for TXD function */
    LPC_IOCON->PIO1_6 = 0x1;
    LPC_IOCON->PIO1_7 = 0x1;

    /* SYSAHBCLKCTRL and UARTCLKDIV handled in SystemInit() */

    /* Disable UART and Clear FIFOs */
    LPC_UART->TER = 0;
    LPC_UART->FCR = (1<<1) | (1<<2);

    /* 13.5.15, pg. 211 of UM10398 LPC111x/LPC11Cxx User manual */
    /* UART Baudrate = PCLK / (16 * DL * (1 + DIVADDVAL/MULVAL))
     *  DL is a 16-bit unsigned integer
     *  1 <= MULVAL <= 15
     *  0 <= DIVADDVAL <= 14
     *  DIVADDVAL < MULVAL
     */

    /* Setup divider */
    /* PCLK = 48MHz */
    /* 115384.62 -- error 184.615 / 0.16%, U0DLM = 0, U0DLL = 15, DIVADDVAL = 11, MULVAL = 15 */
    LPC_UART->LCR = 0x80;
    LPC_UART->DLL = 15;
    LPC_UART->DLM = 0;
    LPC_UART->LCR = 0x00;
    LPC_UART->FDR = (15 << 4) | (11);

    /* Set 8 databits, no parity, 1 stop bit */
    LPC_UART->LCR = 0x3;
    /* Disable flow control */
    LPC_UART->MCR = 0x0;
    /* Disable interrupts */
    LPC_UART->IER = 0x0;

    /* Enable FIFOs */
    LPC_UART->FCR = 0x1;
    /* Enable transmit */
    LPC_UART->TER = 0x80;
}

char uart_getc(void) {
    while (!(LPC_UART->LSR & (1<<0)))
        ;
    return LPC_UART->RBR;
}

void uart_putc(char c) {
    LPC_UART->THR = c;
    while (!(LPC_UART->LSR & (1<<5)))
        ;
}

void uart_puts(const char *s) {
    while (*s != '\0')
        uart_putc(*s++);
}

void uart_gets(char *s, size_t count, bool echo) {
    uint8_t c;
    unsigned int i;

    i = 0;
    while (1) {
        c = uart_getc();

        /* Newline */
        if (c == '\n') {
            if (echo)
                uart_putc(c);
            break;

        /* Backspace/Delete */
        } else if (c == 0x7f || c == 0x08) {
            /* Do nothing for i == 0 */
            if (i == 0)
                continue;

            /* i > 0 */
            i -= 1;
            if (echo)
                uart_putc(0x08);

        /* All other characters */
        } else {
            if (i < count-1)
                s[i++] = c;
            if (echo)
                uart_putc(c);
        }
    }

    s[i] = '\0';
}

void uart_read(uint8_t *buf, size_t count) {
    unsigned int i;

    for (i = 0; i < count; i++)
        buf[i] = uart_getc();
}

void uart_write(const uint8_t *buf, size_t count) {
    unsigned int i;

    for (i = 0; i < count; i++)
        uart_putc(buf[i]);
}

