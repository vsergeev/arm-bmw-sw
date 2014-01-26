#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lpc11xx/LPC11xx.h>

#include "tick.h"
#include "uart.h"
#include "debug.h"
#include "spi.h"
#include "sf.h"
#include "test.h"

#include <tests/tests.h>

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000);

    uart_init();
    spi_init();

    debug_printf("\n\narm-bmw self-test version " STR_VERSION "\n");

    while (1) {
        test_uart();
        test_spi();
        test_spi_flash();
        delay_ms(1000);
    }

    return 0;
}

