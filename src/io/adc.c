#include <stdint.h>

#include <system/lpc11xx/LPC11xx.h>

#include "adc.h"

void adc_init(uint8_t ch_mask) {
    /* Configure PIO0_11, PIO1_0, PIO1_1, PIO1_2, PIO1_3, PIO1_4, PIO1_5 for ADC */
    if (ch_mask & (1<<0))
        LPC_IOCON->R_PIO0_11 = 0x2;
    if (ch_mask & (1<<1))
        LPC_IOCON->R_PIO1_0 = 0x2;
    if (ch_mask & (1<<2))
        LPC_IOCON->R_PIO1_1 = 0x2;
    if (ch_mask & (1<<3))
        LPC_IOCON->R_PIO1_2 = 0x2;
    if (ch_mask & (1<<4))
        LPC_IOCON->SWDIO_PIO1_3 = 0x2;
    if (ch_mask & (1<<5))
        LPC_IOCON->PIO1_4 = 0x2;

    /* Take ADC peripheral out of powerdown */
    LPC_SYSCON->PDRUNCFG &= ~(1<<4);

    /* Disable interrupts for now */
    LPC_ADC->INTEN = 0x0;

    /* ADC Frequency = PCLK / (CLKDIV+1) <= 4.5 MHz
     *  = 48MHz / (10+1) = 4.36 MHz */
    /* Use 10-bit covnersions (CLKS = 0x0) */
    LPC_ADC->CR = (0xa << 8);
}

uint16_t adc_read(uint8_t ch) {
    if (ch > 5)
        return -1;

    /* Preserve CLKDIV, select channel, burst mode, 10-bit conversion, and start */
    /*   7:0 - select channel
     *  15:8 - preserve CLKDIV
     *    16 - 0, burst mode
     * 19:17 - 0, 10-bit conversion
     *    24 - 1, start conversion */
    LPC_ADC->CR = (1<<24) | (LPC_ADC->CR & 0xff00) | (1 << ch);

    /* Wait until ADC conversion completes */
    while (!(LPC_ADC->GDR & (1<<31)))
        ;

    return LPC_ADC->GDR & 0xffff;
}

