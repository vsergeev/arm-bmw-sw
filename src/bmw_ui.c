#include <stdint.h>

#include <io/i2c.h>
#include <driver/mcp23008.h>

#include "bmw_ui.h"

#define GPIO_MASK_LED       0xF0
#define GPIO_MASK_BUTTON    0x0F

struct mcp23008 bmw_iox;

static uint8_t led_state;
static uint8_t button_debounce;
static uint8_t button_state;
static uint8_t button_posedge_state;
static uint8_t button_negedge_state;

#if 0
void EINT0_Handler(void) {
    uint8_t intf, intcap;

    /* Acknowledge GPIO interrupt */
    LPC_GPIO0->IC = (1<<3);

    /* Acknowledge I/O Expander interrupt */
    /* Two reads = 240us at 400KHz I2C */
    mcp23008_reg_read(&bmw_iox, MCP_REG_INTF, &intf);
    mcp23008_reg_read(&bmw_iox, MCP_REG_INTCAP, &intcap);

    /* Check INTF and INTCAP here */
    /* ... */
}
#endif

int bmw_ui_init(void) {
    uint8_t data;

    if (mcp23008_probe(&bmw_iox, &I2C0, 0x20) < 0)
        return -1;

    /* P0-P3 are inputs (Switches), P4-P7 are outputs (LEDs) */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_IODIR, 0x0F) < 0)
        return -1;
    /* Invert button polarity */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_IPOL, 0x0F) < 0)
        return -1;
    #if 0
    /* Enable interrupt-on-change for buttons B1, B0 */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_GPINTEN, 0x0C) < 0)
        return -1;
    #else
    /* Disable interrupt-on-change for now */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_GPINTEN, 0x00) < 0)
        return -1;
    #endif
    /* Reset default value */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_DEFVAL, 0x00) < 0)
        return -1;
    /* Compare to previous pin value for interrupt-on-change */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_INTCON, 0x00) < 0)
        return -1;
    /* Enable INT pin as active-low, open-drain output */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_IOCON, 0x04) < 0)
        return -1;
    /* Disable pull-ups */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_GPPU, 0x00) < 0)
        return -1;
    /* Disable LEDs */
    if (mcp23008_reg_write(&bmw_iox, MCP_REG_GPIO, 0xF0) < 0)
        return -1;

    /* Read current state of buttons */
    if (mcp23008_reg_read(&bmw_iox, MCP_REG_GPIO, &data) < 0)
        return -1;

    /* Initialize LED and Button state */
    led_state = 0xf0;
    button_state = data & 0x0f;
    button_debounce = button_state;
    button_posedge_state = 0;
    button_negedge_state = 0;

    #if 0
    /* Configure IOX_/INT pin as an input */
    LPC_IOCON->PIO0_3 = 0x0;
    LPC_GPIO0->DIR &= ~(1<<3);
    /* Setup as an edge sensitive interrupt */
    LPC_GPIO0->IS &= ~(1<<3);
    /* Trigger on falling edge */
    LPC_GPIO0->IBE &= ~(1<<3);
    LPC_GPIO0->IEV &= ~(1<<3);
    /* Enable interrupt mask */
    LPC_GPIO0->IE |= (1<<3);

    /* Enable interrupt in the NVIC */
    NVIC_ClearPendingIRQ(EINT0_IRQn);
    NVIC_EnableIRQ(EINT0_IRQn);
    NVIC_SetPriority(EINT0_IRQn, 3);
    #endif

    return 0;
}

/* LEDs are active low */

void bmw_ui_led_set(uint8_t state) {
    led_state = (~state) & GPIO_MASK_LED;
    mcp23008_reg_write(&bmw_iox, MCP_REG_GPIO, led_state);
}

void bmw_ui_led_on(uint8_t state) {
    led_state &= ~(state & GPIO_MASK_LED);
    mcp23008_reg_write(&bmw_iox, MCP_REG_GPIO, led_state);
}

void bmw_ui_led_off(uint8_t state) {
    led_state |= (state & GPIO_MASK_LED);
    mcp23008_reg_write(&bmw_iox, MCP_REG_GPIO, led_state);
}

uint8_t bmw_ui_led_state(void) {
    return ~led_state & GPIO_MASK_LED;
}

/* Buttons are active high (due to IPOL setting) */

void bmw_ui_button_debounce(void) {
    uint8_t data, stable, next_button_state;

    /* Read current button state */
    mcp23008_reg_read(&bmw_iox, MCP_REG_GPIO, &data);

    /* Find stable bits in current data and debounced data */
    stable = ~(data ^ button_debounce) & GPIO_MASK_BUTTON;
    /* Update debounced data */
    button_debounce = ((button_debounce & stable) | (data & ~stable)) & GPIO_MASK_BUTTON;
    /* Prepare next button state from debounced stable bits */
    next_button_state = ((button_state & ~stable) | (data & stable)) & GPIO_MASK_BUTTON;

    /* Find new positive edges */
    button_posedge_state |= (button_state ^ next_button_state) & (next_button_state);
    /* Find new negative edges */
    button_negedge_state |= (button_state ^ next_button_state) & ~(next_button_state);

    /* Update button state */
    button_state = next_button_state;
}

uint8_t bmw_ui_button_state(void) {
    return button_state;
}

uint8_t bmw_ui_button_posedge(void) {
    uint8_t state = button_posedge_state;
    button_posedge_state = 0;
    return state;
}

uint8_t bmw_ui_button_negedge(void) {
    uint8_t state = button_negedge_state;
    button_negedge_state = 0;
    return state;
}

