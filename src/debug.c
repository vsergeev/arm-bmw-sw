#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <io/uart.h>

#include "debug.h"

/* Basic printf. Supports conversions 'bxXducs%', space padding, zero padding. */

static char *format32_unsigned(char *buf, size_t len, uint32_t value, int radix, const char *map) {
    unsigned int i = len-1;

    buf[i] = '\0';
    do {
        buf[--i] = map[value % radix];
        value /= radix;
    } while (value != 0 && i > 0);

    return buf + i;
}

static char *format32_signed(char *buf, size_t len, int32_t value, int radix, const char *map) {
    unsigned int i = len-1;
    bool sign;

    if (value < 0) {
        sign = true;
        value = -value;
    } else {
        sign = false;
    }

    buf[i] = '\0';
    do {
        buf[--i] = map[value % radix];
        value /= radix;
    } while (value != 0 && i > 0);

    if (sign && i > 0)
        buf[--i] = '-';

    return buf + i;
}

#define _isdigit(c) ((c >= '0') && (c <= '9'))
#define _char2digit(c) (c - '0')
enum printf_state { STATE_COPY, STATE_FORMAT, STATE_PADDING };

void debug_printf(const char *fmt, ...) {
    va_list ap;
    enum printf_state state = STATE_COPY;
    char buf[64], *p;
    char charpad = ' ';
    int numpad = 0;

    va_start(ap, fmt);

    for (; *fmt != '\0'; fmt++) {
        if (state == STATE_COPY) {
            if (*fmt == '%') {
                state = STATE_FORMAT;
                numpad = 0;
                continue;
            } else {
                uart_putc(*fmt);
                continue;
            }
        } else if (state == STATE_FORMAT && (*fmt == ' ' || *fmt == '0')) {
            state = STATE_PADDING;
            charpad = *fmt;
            continue;
        } else if (state == STATE_PADDING) {
            if (_isdigit(*fmt)) {
                numpad = numpad*10 + _char2digit(*fmt);
                continue;
            } else {
                state = STATE_FORMAT;
            }
        }

        if (state == STATE_FORMAT) {
            if (*fmt == '%') {
                p = "%";
                numpad = 0;
            } else if (*fmt == 'b') {
                uint32_t value = va_arg(ap, uint32_t);
                p = format32_unsigned(buf, sizeof(buf), value, 2, "01");
                numpad -= strlen(p);
            } else if (*fmt == 'x') {
                uint32_t value = va_arg(ap, uint32_t);
                p = format32_unsigned(buf, sizeof(buf), value, 16, "0123456789abcdef");
                numpad -= strlen(p);
            } else if (*fmt == 'X') {
                uint32_t value = va_arg(ap, uint32_t);
                p = format32_unsigned(buf, sizeof(buf), value, 16, "0123456789ABCDEF");
                numpad -= strlen(p);
            } else if (*fmt == 'd') {
                int32_t value = va_arg(ap, int32_t);
                p = format32_signed(buf, sizeof(buf), value, 10, "0123456789");
                numpad -= strlen(p);
            } else if (*fmt == 'u') {
                uint32_t value = va_arg(ap, uint32_t);
                p = format32_unsigned(buf, sizeof(buf), value, 10, "0123456789");
                numpad -= strlen(p);
            } else if (*fmt == 'p') {
                uintptr_t value = va_arg(ap, uintptr_t);
                p = format32_unsigned(buf, sizeof(buf), value, 16, "0123456789abcdef");
                charpad = '0';
                numpad = 8 - strlen(p);
            } else if (*fmt == 'c') {
                char value = va_arg(ap, int);
                buf[0] = value;
                buf[1] = '\0';
                p = buf;
                numpad = 0;
            } else if (*fmt == 's') {
                char *value = va_arg(ap, char *);
                p = value;
                numpad = 0;
            } else {
                p = "";
                numpad = 0;
            }

            for (; numpad > 0; numpad--)
                uart_putc(charpad);
            uart_puts(p);

            state = STATE_COPY;
        }
    }
}

/* Small test
    debug_printf("test [%b] [%04b] [% 4b] [%x] [%X] [%d] [%d] [%u] [%u] [%c] [\"%s\"] [%%]\n", 0xaa, 0x3, 0x3, 0xabcdcafe, 0xabcdcafe, 123, -123, 100, -1, 'a', "test");
 -> test [10101010] [0011] [  11] [abcdcafe] [ABCDCAFE] [123] [-123] [100] [4294967295] [a] ["test"] [%]
 */

