#include <stdint.h>
#include <stdlib.h>
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
        pinteract("UART getc() works?");
    } while(1);

    debug_printf(STR_TAB "enter the string banana: ");
    uart_gets(buf, sizeof(buf), true);
    if (strcmp(buf, "banana") == 0)
        pokay("UART gets() works, got banana");
    else
        pfail("expected banana, got \"%s\"", buf);

    pokay("fancy printf 0x%04x 0x%04X 0b%08b %d %u \"%s\"", 0xcafe, 0xcafe, 0x55, -1, -1, "test");
}

void _test_spi_transfer(struct spi_slave *spi, const char *prompt) {
    uint8_t buf[4] = {0xaa, 0x55, 0xf0, 0x0f};

    debug_printf("\n");
    debug_printf(STR_TAB "Now testing transfer: %s\n", prompt);

    do {
        debug_printf(STR_TAB "Press enter to start transfer...");
        uart_getc(); uart_puts("\n");

        spi_setup(spi);
        spi_select(spi);
        spi_transfer(spi, buf, NULL, sizeof(buf));
        spi_deselect(spi);

        pinteract("%s occurred?", prompt);
    } while(1);
}

void test_spi(void) {
    struct spi_slave spi;
    uint8_t txbuf[1024], rxbuf[1024];
    unsigned int i;

    ptest();

    /* Enable GPIO12 / PIO1.0 for CS */
    LPC_IOCON->R_PIO1_0 = 0x1;
    LPC_GPIO1->DIR |= (1<<0);

    /* Setup SPI slave */
    spi.master = &SPI0;
    spi.cs_pin = 12;
    spi.cs_active_high = false;
    spi.speed = 1000000;

    spi.mode = 0;
    _test_spi_transfer(&spi, "SPI mode=0, speed=1e6, cs active low");

    spi.mode = 1;
    _test_spi_transfer(&spi, "SPI mode=1, speed=1e6, cs active low");

    spi.mode = 2;
    _test_spi_transfer(&spi, "SPI mode=2, speed=1e6, cs active low");

    spi.mode = 3;
    _test_spi_transfer(&spi, "SPI mode=3, speed=1e6, cs active low");

    spi.mode = 0;
    spi.cs_active_high = true;
    _test_spi_transfer(&spi, "SPI mode=0, speed=1e6, cs active high");

    spi.mode = 0;
    spi.cs_active_high = false;
    spi.speed = 6000000;
    _test_spi_transfer(&spi, "SPI mode=0, speed=6e6, cs active low");

    spi.speed = 12000000;
    _test_spi_transfer(&spi, "SPI mode=0, speed=12e6, cs active low");

    spi.speed = 24000000;
    _test_spi_transfer(&spi, "SPI mode=0, speed=24e6, cs active low");

    debug_printf("\n");
    debug_printf(STR_TAB "Generating pseudorandom txbuf:\n");
    srand(0xdeadbeef);
    for (i = 0; i < sizeof(txbuf); i++) {
        txbuf[i] = (uint8_t)rand();
        rxbuf[i] = 0xff;
        debug_printf("%02x ", txbuf[i]);
    }
    debug_printf("\n");

    spi.speed = 1000000;
    debug_printf(STR_TAB "Please tie MISO and MOSI for loopback test and press enter...");
    uart_getc(); uart_puts("\n");

    spi_setup(&spi);
    spi_select(&spi);
    spi_transfer(&spi, txbuf, rxbuf, sizeof(txbuf));
    spi_deselect(&spi);

    if (memcmp(txbuf, rxbuf, sizeof(txbuf)) == 0)
        pokay("SPI mode=0, speed=1e6 loopback success!");
    else {
        pfail("SPI mode=0, speed=1e6 loopback failed! Got back:");
        for (i = 0; i < sizeof(rxbuf); i++)
            debug_printf("%02x ", rxbuf[i]);
        debug_printf("\n");
    }
}

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
        delay_ms(1000);
    }

    return 0;
}

