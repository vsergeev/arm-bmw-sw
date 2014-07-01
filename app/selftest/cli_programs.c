#include <stdlib.h>
#include <string.h>

#include <system/tick.h>
#include <io/spi.h>
#include <io/adc.h>
#include <io/i2c_reg.h>
#include <io/uart.h>
#include <driver/sf.h>
#include <debug.h>
#include <test.h>
#include <bmw_ui.h>
#include <ucli.h>
#include <urpc.h>

#include "tests/tests.h"

static void prog_help(int argc, char **argv) {
    debug_printf("Programs available\n");
    debug_printf("\thelp            show this help\n");
    debug_printf("\tversion         print firmware version\n");
    debug_printf("\ttest            run a built-in test\n");
    debug_printf("\tled             turn on/off LEDs\n");
    debug_printf("\tadc             read ADC channels\n");
    debug_printf("\tbuttons         read buttons\n");
    debug_printf("\ttime            read system time\n");
    debug_printf("\tsf              probe, read spi flash\n");
    debug_printf("\ti2c             detect, read, write i2c devices\n");
    debug_printf("\trpc_server      run urpc server\n");
}

static void prog_version(int argc, char **argv) {
    debug_printf(GIT_VERSION "\n");
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

    uart_flush();
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
    uint8_t c = 0;

    debug_printf("Press 'q' to quit.\n");
    do {
        bmw_ui_button_debounce();
        state = bmw_ui_button_state();

        debug_printf("\rBT0: %b  BT1: %b  SW0: %b  SW1: %b  ", !!(state & BMW_BT0), !!(state & BMW_BT1), !!(state & BMW_SW0), !!(state & BMW_SW1));
        delay_ms(50);
        if (uart_poll())
            c = uart_getc();
    } while (c != 'q');
    debug_printf("\n");
}

static void prog_adc(int argc, char **argv) {
    unsigned int i;
    uint8_t c = 0;
    uint16_t uval[5];

    /* Initialize all five channels */
    adc_init(0x1f);

    debug_printf("Press 'q' to quit.\n");
    do {
        for (i = 0; i < 5; i++)
            uval[i] = adc_read(i);

        debug_printf("\r%05u %05u %05u %05u %05u", uval[0], uval[1], uval[2], uval[3], uval[4]);

        if (uart_poll())
            c = uart_getc();
    } while (c != 'q');
    debug_printf("\n");
}

static void prog_time(int argc, char **argv) {
    debug_printf("system time: %d ms\n", time());
}

static const struct spi_slave sf_spi = { .master = &SPI0, .speed = 24000000, .mode = 0, .cs_pin = 2, .cs_active_high = false };
static struct spi_flash sf = { .spi = NULL, .params = NULL };

static void prog_sf(int argc, char **argv) {
    uint8_t buf[256];
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

static void prog_i2c(int argc, char **argv) {
    uint8_t address, reg, data;
    int ret;

    if (argc < 2) {
        debug_printf("Usage: %s scan\n", argv[0]);
        debug_printf("Usage: %s detect <address>\n", argv[0]);
        debug_printf("Usage: %s read <address> <register>\n", argv[0]);
        debug_printf("Usage: %s write <address> <register> <data>\n", argv[0]);
        return;
    }

    if (strcmp(argv[1], "scan") == 0) {
        bool alive[128];
        int i;

        memset(alive, 0, sizeof(alive));
        for (i = 0; i < 128; i++) {
            if (i2c_detect(&I2C0, i) == 0)
                alive[i] = true;
        }

        debug_printf("Detected I2C devices at addresses:\n");
        for (i = 0; i < 128; i++) {
            if (alive[i])
                debug_printf("    0x%02x\n", i);
        }

    } else if (strcmp(argv[1], "detect") == 0) {
        if (argc < 3) {
            debug_printf("missing arguments\n");
            return;
        }

        address = strtoul(argv[2], NULL, 0);
        ret = i2c_detect(&I2C0, address);
        debug_printf("i2c_detect(): %d\n", ret);
        if (ret == 0)
            pokay("detect 0x%02x success", address);
        else
            pfail("detect 0x%02x failure");

    } else if (strcmp(argv[1], "read") == 0) {
        if (argc < 4) {
            debug_printf("missing arguments\n");
            return;
        }

        address = strtoul(argv[2], NULL, 0);
        reg = strtoul(argv[3], NULL, 0);
        ret = i2c_reg_read(&I2C0, address, reg, &data);
        debug_printf("i2c_reg_read(): %d\n", ret);
        if (ret == 0)
            pokay("read success, 0x%02x: 0x%02x", reg, data);
        else
            pokay("read failure\n");

    } else if (strcmp(argv[1], "write") == 0) {
        if (argc < 5) {
            debug_printf("missing arguments\n");
            return;
        }

        address = strtoul(argv[2], NULL, 0);
        reg = strtoul(argv[3], NULL, 0);
        data = strtoul(argv[4], NULL, 0);
        ret = i2c_reg_write(&I2C0, address, reg, data);
        if (ret == 0)
            pokay("write success");
        else
            pokay("write failure");

    }
}

static void prog_rpc_server(int argc, char **argv) {
    urpc_server();
}

const struct ucli_program CLI_Programs[] = {
    { .name = "help", .func = prog_help },
    { .name = "version", .func = prog_version },
    { .name = "test", .func = prog_test },
    { .name = "led", .func = prog_led },
    { .name = "buttons", .func = prog_buttons },
    { .name = "adc", .func = prog_adc },
    { .name = "time", .func = prog_time },
    { .name = "i2c", .func = prog_i2c },
    { .name = "sf", .func = prog_sf },
    { .name = "rpc_server", .func = prog_rpc_server },
    { .name = NULL, .func = NULL },
};

