#include <stdlib.h>
#include <string.h>

#include "debug.h"

#include "tick.h"
#include "bmw_ui.h"
#include "spi.h"
#include "sf.h"

#include "tests/tests.h"

#include "cli.h"

static void prog_help(int argc, char **argv) {
    debug_printf("\nPrograms available\n");
    debug_printf("\thelp            show this help\n");
    debug_printf("\ttest            run a built-in test\n");
    debug_printf("\tled             manipulate LEDs\n");
    debug_printf("\tbuttons         read buttons\n");
    debug_printf("\ttime            read system time\n");
    debug_printf("\tsf              read spi flash\n");
    debug_printf("\n");
}

static void prog_test(int argc, char **argv) {
    if (argc < 2) {
        debug_printf("Usage: %s <test>\n", argv[0]);
        debug_printf("Tests available\n");
        debug_printf("\tuart\n");
        debug_printf("\tspi\n");
        debug_printf("\tspi_flash\n");
        debug_printf("\ti2c\n");
        debug_printf("\tmcp23008\n");
        debug_printf("\tbmw_ui\n");
        return;
    }

    if (strcmp(argv[1], "uart") == 0)
        test_uart();
    else if (strcmp(argv[1], "spi") == 0)
        test_spi();
    else if (strcmp(argv[1], "spi_flash") == 0)
        test_spi_flash();
    else if (strcmp(argv[1], "i2c") == 0)
        test_i2c();
    else if (strcmp(argv[1], "mcp23008") == 0)
        test_mcp23008();
    else if (strcmp(argv[1], "bmw_ui") == 0)
        test_bmw_ui();
    else
        debug_printf("unknown test \"%s\"\n", argv[1]);
}

static void prog_led(int argc, char **argv) {
    unsigned int led;
    uint8_t led_bit_map[] = { [1] = BMW_LED1, [2] = BMW_LED2, [3] = BMW_LED3, [4] = BMW_LED4 };

    if (argc < 3) {
        debug_printf("Usage: %s <on/off> <LED number>\n", argv[0]);
        return;
    }

    led = strtoul(argv[2], NULL, 10);
    if (led < 1 || led > 4) {
        debug_printf("invalid LED number\n");
        return;
    }

    if (strcmp(argv[1], "on") == 0)
        bmw_ui_led_on(led_bit_map[led]);
    else if (strcmp(argv[1], "off") == 0)
        bmw_ui_led_off(led_bit_map[led]);
    else
        debug_printf("invalid LED command\n");
}

static void prog_buttons(int argc, char **argv) {
    uint8_t state;

    bmw_ui_button_debounce();
    bmw_ui_button_debounce();
    state = bmw_ui_button_state();

    debug_printf("BT0:  %b\n", !!(state & BMW_BT0));
    debug_printf("BT1:  %b\n", !!(state & BMW_BT1));
    debug_printf("SW0:  %b\n", !!(state & BMW_SW0));
    debug_printf("SW1:  %b\n", !!(state & BMW_SW1));
}

static void prog_time(int argc, char **argv) {
    debug_printf("system time: %d ms\n", time());
}

static struct spi_slave sf_spi = { .master = &SPI0, .speed = 24000000, .mode = 0, .cs_pin = 2, .cs_active_high = false };
static struct spi_flash sf = { .spi = NULL, .params = NULL };

static void prog_sf(int argc, char **argv) {
    char buf[256];
    uint32_t address, length, length_read;
    unsigned int i;

    if (argc < 2) {
        debug_printf("Usage: %s probe\n", argv[0]);
        debug_printf("Usage: %s read [address=0] [length=256]\n", argv[0]);
        return;
    }

    address = (argc > 2) ? strtoul(argv[2], NULL, 0) : 0;
    length = (argc > 3) ? strtoul(argv[3], NULL, 0) : 256;

    if (strcmp(argv[1], "probe") == 0) {
        debug_printf("spi_flash_probe(): %d\n", spi_flash_probe(&sf, &sf_spi));
    } else if (strcmp(argv[1], "read") == 0) {
        if (sf.spi == NULL) {
            debug_printf("Error: please probe SPI flash first.\n");
            return;
        }

        for (length_read = 0; length_read < length; ) {
            size_t rlen = ((length-length_read) < sizeof(buf)) ? (length-length_read) : sizeof(buf);
            spi_flash_read(&sf, address+length_read, buf, rlen);
            length_read += rlen;

            for (i = 0; i < rlen; i++) {
                debug_printf("%02x ", buf[i]);
                if (((i+1) % 32) == 0)
                    debug_printf("\n");
                else if (((i+1) % 8) == 0)
                    debug_printf(" ");
            }
        }
        if ((length_read % 32) != 0)
            debug_printf("\n");

    } else {
        debug_printf("unknown sf command\n");
    }
}

struct cli_program cli_programs[] = {
    { .name = "help", .func = prog_help },
    { .name = "test", .func = prog_test },
    { .name = "led", .func = prog_led },
    { .name = "buttons", .func = prog_buttons },
    { .name = "time", .func = prog_time },
    { .name = "sf", .func = prog_sf },
    { .name = NULL, .func = NULL },
};

