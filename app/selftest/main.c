#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <system/lpc11xx/LPC11xx.h>
#include <system/tick.h>

#include <io/uart.h>
#include <io/spi.h>
#include <io/i2c.h>

#include <debug.h>
#include <test.h>
#include <bmw_ui.h>
#include <ucli.h>

#include "tests/tests.h"

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000);

    uart_init();
    spi_init();
    i2c_init();
    bmw_ui_init();

    /* Enable PIO0.2 for SPI Flash CS */
    LPC_GPIO0->DIR |= (1<<2);
    LPC_GPIO0->DATA |= (1<<2);

    debug_printf("\n\narm-bmw self-test version " "\x1b[1;33m" GIT_VERSION "\x1b[0m" "\n\n");

    #ifdef NO_CLI
    while (1) {
        test_uart();
        test_spi();
        test_spi_flash();
        test_i2c();
        test_mcp23008();
        test_bmw_ui();
        delay_ms(1000);
    }
    #else
    ucli_server();
    #endif

    return 0;
}

