#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void uart_init(void);
char uart_getc(void);
void uart_putc(char c);
bool uart_poll(void);
void uart_puts(const char *s);
void uart_gets(char *s, size_t len, bool echo);
void uart_read(uint8_t *buf, size_t count);
void uart_write(const uint8_t *buf, size_t count);

#endif

