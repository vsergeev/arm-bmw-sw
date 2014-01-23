#ifndef _TEST_H
#define _TEST_H

#include "debug.h"

#define STR_OK          "[\x1b[1;32m OK \x1b[0m]"
#define STR_FAIL        "[\x1b[1;31mFAIL\x1b[0m]"
#define STR_INTERACT    "[\x1b[1;36m    \x1b[0m]"

#ifdef GIT_VERSION
#define STR_VERSION     "\x1b[1;33m" GIT_VERSION "\x1b[0m"
#else
#define STR_VERSION     "\x1b[1;33munknown\x1b[0m"
#endif

#define passert(c) \
    do { \
        int r = (c); \
        if (r) \
            debug_printf(" " STR_OK "  %s():%d  %s\n", __func__, __LINE__, #c); \
        else {\
            debug_printf(" " STR_FAIL "  %s():%d  %s\n", __func__, __LINE__, #c); \
            return; \
        } \
    } while(0)

#define pokay(fmt, ...) \
    debug_printf(" " STR_OK "  " fmt "\n", ##__VA_ARGS__)

#define pfail(fmt, ...) \
    debug_printf(" " STR_FAIL "  " fmt "\n", ##__VA_ARGS__)

#define ptest() \
    debug_printf("\nStarting test %s() (%s:%d)\n", __func__, __FILE__, __LINE__)

#define pinteract(fmt, ...) \
    { \
        char c; \
        debug_printf(" " STR_INTERACT "  " fmt "? (y/n/r) ", ##__VA_ARGS__); \
        c = uart_getc(); \
        uart_putc('\n'); \
        if (c == 'y' || c == 'Y') { \
            pokay(fmt, ##__VA_ARGS__); \
            break; \
        } else if (c == 'n' || c == 'N') { \
            pfail(fmt, ##__VA_ARGS__); \
            break; \
        } else { \
            continue; \
        } \
    }

#endif

