#include <stdint.h>

#include <system/lpc11xx/LPC11xx.h>

#include "tick.h"

volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

uint32_t time(void) {
    return msTicks;
}

void delay_ms(uint32_t ms) {
    uint32_t now = msTicks;
    while ((msTicks-now) < ms)
        __WFI();
}

