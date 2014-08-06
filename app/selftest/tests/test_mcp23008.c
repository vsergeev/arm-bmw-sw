#include <stdlib.h>
#include <string.h>

#include <system/tick.h>
#include <io/uart.h>
#include <io/i2c.h>
#include <driver/mcp23008.h>

#include <debug.h>
#include <test.h>

void test_mcp23008(void) {
    struct mcp23008 mcp;
    int state;

    ptest();

    debug_printf(STR_TAB "Press enter to start MCP23008 test...");
    uart_getc(); uart_putc('\n');

    /* Probe for the I/O expander */
    debug_printf(STR_TAB "Probing for MCP23008...\n");
    passert(mcp23008_probe(&mcp, &I2C0, 0x20) == 0);
    pokay("Found MCP23008!");

    debug_printf(STR_TAB "Initializing P4-P7 to outputs with value 1\n");
    passert(mcp23008_reg_write(&mcp, MCP_REG_OLAT, 0xf0) == 0);
    passert(mcp23008_reg_write(&mcp, MCP_REG_IODIR, 0x0f) == 0);
    passert(mcp23008_reg_write(&mcp, MCP_REG_IPOL, 0x00) == 0);

    debug_printf(STR_TAB "Press enter to turn on LED1...");
    uart_getc(); uart_putc('\n');
    passert(mcp23008_reg_write(&mcp, MCP_REG_GPIO, ~(1<<4) & 0xf0) == 0);
    do { pinteract("LED1 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on LED2...");
    uart_getc(); uart_putc('\n');
    passert(mcp23008_reg_write(&mcp, MCP_REG_GPIO, ~(1<<5) & 0xf0) == 0);
    do { pinteract("LED2 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on LED3...");
    uart_getc(); uart_putc('\n');
    passert(mcp23008_reg_write(&mcp, MCP_REG_GPIO, ~(1<<6) & 0xf0) == 0);
    do { pinteract("LED3 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on LED4...");
    uart_getc(); uart_putc('\n');
    passert(mcp23008_reg_write(&mcp, MCP_REG_GPIO, ~(1<<7) & 0xf0) == 0);
    do { pinteract("LED4 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on all LEDs...");
    uart_getc(); uart_putc('\n');
    passert(mcp23008_reg_write(&mcp, MCP_REG_GPIO, 0x00) == 0);
    do { pinteract("All LEDs on?"); } while(0);

    debug_printf(STR_TAB "Polling GPIOs\n");
    state = 0;
    while (1) {
        uint8_t data;

        if (state == 0) {
            debug_printf(STR_TAB "Waiting for button 1 press...\n");
            state++;
        } else if (state == 2) {
            debug_printf(STR_TAB "Waiting for button 2 press...\n");
            state++;
        } else if (state == 4) {
            debug_printf(STR_TAB "Waiting for switch 1 on...\n");
            state++;
        } else if (state == 6) {
            debug_printf(STR_TAB "Waiting for switch 2 on...\n");
            state++;
        } else if (state == 8) {
            break;
        }

        if (mcp23008_reg_read(&mcp, MCP_REG_GPIO, &data) < 0) {
            pfail("mcp23008_reg_read() failed");
            passert(false);
        }

        if (state == 1 && !(data & (1<<2))) {
            pokay("Got button 1 press!");
            state++;
        } else if (state == 3 && !(data & (1<<3))) {
            pokay("Got button 2 press!");
            state++;
        } else if (state == 5 && !(data & (1<<0))) {
            pokay("Got switch 1 on!");
            state++;
        } else if (state == 7 && !(data & (1<<1))) {
            pokay("Got switch 2 on!");
            state++;
        }

        delay_ms(25);
    }
}

