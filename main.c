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
    SysTick_Config(SystemCoreClock/1000);

    LPC_GPIO1->DIR = (1<<3)|(1<<2)|(1<<1)|(1<<0);

    while (1) {
        LPC_GPIO1->DATA = (1<<3)|(1<<2)|(1<<1)|(1<<0);
        delay_ms(500);
        LPC_GPIO1->DATA = 0;
        delay_ms(500);
    }

    return 0;
}

