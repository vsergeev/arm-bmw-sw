#ifndef _TICK_H
#define _TICK_H

#include <stdint.h>

extern volatile uint32_t msTicks;

void delay_ms(uint32_t ms);

#endif

