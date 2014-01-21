#include <stdint.h>

#include <lpc11xx/LPC11xx.h>

#include "uart.h"
#include "debug.h"

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

    #if 1
    /* Main clock on P0_1 for debug */
    LPC_SYSCON->CLKOUTCLKSEL = 3;
    LPC_SYSCON->CLKOUTDIV = 1;
    LPC_SYSCON->CLKOUTUEN = 0;
    LPC_SYSCON->CLKOUTUEN = 1;
    LPC_IOCON->PIO0_1 = 0x1;
    #endif

    uart_init();

    char buf[32];

    while (1) {
        uart_puts("type something: ");
        uart_gets(buf, sizeof(buf));
        uart_putc('\n');
        debug_printf("got _%s_\n", buf);
        delay_ms(250);
    }

    return 0;
}

