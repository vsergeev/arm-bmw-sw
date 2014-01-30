#include <stdlib.h>
#include <string.h>

#include <bmw_ui.h>

#include <tick.h>
#include <uart.h>
#include <test.h>

void test_bmw_ui(void) {

    ptest();

    debug_printf(STR_TAB "Initializing BMW UI\n");
    passert(bmw_ui_init() == 0);

    debug_printf(STR_TAB "Press enter to turn on LED1...");
    uart_getc(); uart_putc('\n');
    bmw_ui_led_on(BMW_LED1);
    do { pinteract("LED1 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on LED2...");
    uart_getc(); uart_putc('\n');
    bmw_ui_led_off(BMW_LED1);
    bmw_ui_led_on(BMW_LED2);
    do { pinteract("LED2 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on LED3...");
    uart_getc(); uart_putc('\n');
    bmw_ui_led_off(BMW_LED2);
    bmw_ui_led_on(BMW_LED3);
    do { pinteract("LED3 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on LED4...");
    uart_getc(); uart_putc('\n');
    bmw_ui_led_off(BMW_LED3);
    bmw_ui_led_on(BMW_LED4);
    do { pinteract("LED4 on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn on all LEDs...");
    uart_getc(); uart_putc('\n');
    bmw_ui_led_set(BMW_LED1 | BMW_LED2 | BMW_LED3 | BMW_LED4);
    do { pinteract("All LEDs on?"); } while(0);

    debug_printf(STR_TAB "Press enter to turn off all LEDs...");
    uart_getc(); uart_putc('\n');
    bmw_ui_led_set(0);
    do { pinteract("All LEDs off?"); } while(0);

    debug_printf(STR_TAB "Waiting to reset all switches to off\n");
    do {
        bmw_ui_button_debounce();
        delay_ms(1);
    } while (bmw_ui_button_state() != 0);
    pokay("Switches reset to 0");

    debug_printf(STR_TAB "Waiting for posedge on button 0\n");
    do {
        bmw_ui_button_debounce();
        delay_ms(1);
    } while ((bmw_ui_button_posedge() & BMW_BT0) == 0);
    pokay("Posedge on button 0");

    debug_printf(STR_TAB "Waiting for negedge on button 0\n");
    do {
        bmw_ui_button_debounce();
        delay_ms(1);
    } while ((bmw_ui_button_negedge() & BMW_BT0) == 0);
    pokay("Negedge on button 0");

    debug_printf(STR_TAB "Waiting for button 0 and button 1 on\n");
    do {
        bmw_ui_button_debounce();
        delay_ms(1);
    } while ((bmw_ui_button_state() & (BMW_BT0 | BMW_BT1)) != (BMW_BT0 | BMW_BT1));
    pokay("Buttons 0 and 1 high");

    debug_printf(STR_TAB "Waiting for posedge on switch 0\n");
    do {
        bmw_ui_button_debounce();
        delay_ms(1);
    } while ((bmw_ui_button_posedge() & BMW_SW0) == 0);
    pokay("Posedge on switch 0");

    debug_printf(STR_TAB "Waiting for negedge on switch 0\n");
    do {
        bmw_ui_button_debounce();
        delay_ms(1);
    } while ((bmw_ui_button_negedge() & BMW_SW0) == 0);
    pokay("Negedge on switch 0");

    pokay("All tests passed!");
}

