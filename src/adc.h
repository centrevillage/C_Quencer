#include <avr/io.h>

#ifndef CQ_ADC_H_
#define CQ_ADC_H_

void adc_init();
uint8_t adc_read(uint8_t pin);

#endif /* CQ_ADC_H_ */
