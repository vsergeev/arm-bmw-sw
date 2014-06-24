#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>

void adc_init(uint8_t ch_mask);
uint16_t adc_read(uint8_t ch);

#endif

