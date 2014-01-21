#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include <stddef.h>

void uart_init(void);
char uart_getc(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_gets(char *s, size_t count);
void uart_read(uint8_t *buf, size_t count);
void uart_write(const uint8_t *buf, size_t count);

#endif

