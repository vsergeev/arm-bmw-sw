#include <stdint.h>

#include "LPC11xx.h"

volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

void delay_ms(uint32_t ms) {
    uint32_t now = msTicks;
    while ((msTicks-now) < ms)
        ;
}

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();
    #if 1
    /* Main clock on P0_1 for debug */
    LPC_SYSCON->CLKOUTCLKSEL = 3;
    LPC_SYSCON->CLKOUTDIV = 1;
    LPC_SYSCON->CLKOUTUEN = 0;
    LPC_SYSCON->CLKOUTUEN = 1;
    LPC_IOCON->PIO0_1 = 0x1;
    #endif
    SysTick_Config(SystemCoreClock/1000);

    LPC_GPIO0->DIR = (1<<7);
    while (1) {
        LPC_GPIO0->DATA = (1<<7);
        delay_ms(125);
        LPC_GPIO0->DATA = 0;
        delay_ms(125);
    }

    return 0;
}

