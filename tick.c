#include <stdint.h>
#include <lpc11xx/LPC11xx.h>

#include "tick.h"

volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

void delay_ms(uint32_t ms) {
    uint32_t now = msTicks;
    while ((msTicks-now) < ms)
        __WFI();
}

