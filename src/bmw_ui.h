#ifndef _BMW_UI_H
#define _BMW_UI_H

#include <stdint.h>

#define BMW_SW0     (1<<0)
#define BMW_SW1     (1<<1)
#define BMW_BT0     (1<<2)
#define BMW_BT1     (1<<3)
#define BMW_LED1    (1<<4)
#define BMW_LED2    (1<<5)
#define BMW_LED3    (1<<6)
#define BMW_LED4    (1<<7)

int bmw_ui_init(void);

void bmw_ui_led_set(uint8_t state);
void bmw_ui_led_on(uint8_t state);
void bmw_ui_led_off(uint8_t state);
uint8_t bmw_ui_led_state(void);

void bmw_ui_button_debounce(void);
uint8_t bmw_ui_button_state(void);
uint8_t bmw_ui_button_posedge(void);
uint8_t bmw_ui_button_negedge(void);

#endif

