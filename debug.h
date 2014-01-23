#ifndef _DEBUG_H
#define _DEBUG_H

void debug_printf(const char *fmt, ...);

#endif

#ifdef DEBUG
    #undef dbg
    #define dbg     debug_printf
#else
    #undef dbg
    #define dbg(fmt, ...)
#endif

