#include <stdlib.h>
#include <string.h>

#include <io/uart.h>

#include <debug.h>
#include <test.h>

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

    pokay("All tests passed!");
}

