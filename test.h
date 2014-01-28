#ifndef _TEST_H
#define _TEST_H

#include "debug.h"

#define STR_OKAY        " [\x1b[1;32m OK \x1b[0m]  "
#define STR_FAIL        " [\x1b[1;31mFAIL\x1b[0m]  "
#define STR_TAB         "         "

#ifdef GIT_VERSION
#define STR_VERSION     "\x1b[1;33m" GIT_VERSION "\x1b[0m"
#else
#define STR_VERSION     "\x1b[1;33munknown\x1b[0m"
#endif

#define passert(c) \
    do { \
        int r = (c); \
        if (r) \
            debug_printf(STR_OKAY "%s():%d  %s\n", __func__, __LINE__, #c); \
        else {\
            debug_printf(STR_FAIL "%s():%d  %s\n", __func__, __LINE__, #c); \
            return; \
        } \
    } while(0)

#define pokay(fmt, ...) \
    do { \
        uart_puts(STR_OKAY); \
        debug_printf(fmt, ##__VA_ARGS__); \
        uart_puts("\n"); \
    } while (0)

#define pfail(fmt, ...) \
    do { \
        uart_puts(STR_FAIL); \
        debug_printf(fmt, ##__VA_ARGS__); \
        uart_puts("\n"); \
    } while (0)

#define ptest() \
    debug_printf("\nStarting test %s() (%s:%d)\n", __func__, __FILE__, __LINE__)

#define pinteract(fmt, ...) \
    { \
        char _buf[2]; \
        uart_puts(STR_TAB); \
        debug_printf(fmt, ##__VA_ARGS__); \
        uart_puts(" (y/n/r) "); \
        uart_gets(_buf, sizeof(_buf), true); \
        if (_buf[0] == 'y' || _buf[0] == 'Y') { \
            pokay(fmt, ##__VA_ARGS__); \
            break; \
        } else if (_buf[0] == 'n' || _buf[0] == 'N') { \
            pfail(fmt, ##__VA_ARGS__); \
            break; \
        } else { \
            continue; \
        } \
    }

#endif

