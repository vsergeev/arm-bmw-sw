#include <stdint.h>
#include <string.h>
#include <lpc11xx/LPC11xx.h>

#include "tick.h"
#include "uart.h"
#include "debug.h"
#include "spi.h"
#include "test.h"

void test_uart(void) {
    char buf[32];

    ptest();

    pokay("UART puts() works");

    do {
        pinteract("UART getc() works");
    } while(1);

    debug_printf("enter the string banana: ");
    uart_gets(buf, sizeof(buf), true);
    if (strcmp(buf, "banana") == 0)
        pokay("UART gets() works, got banana");
    else
        pfail("expected banana, got \"%s\"", buf);

    pokay("fancy printf 0x%04x 0x%04X 0b%08b %d %u \"%s\"", 0xcafe, 0xcafe, 0x55, -1, -1, "test");
}

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000);

    uart_init();

    debug_printf("\n\narm-bmw self-test version " STR_VERSION "\n");

    while (1) {
        test_uart();
        delay_ms(1000);
    }

    return 0;
}

