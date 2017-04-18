#include <avr/io.h>

#ifndef CQ_ADC_H_
#define CQ_ADC_H_

typedef void (*AdcAsyncReadCallback)(uint8_t, uint8_t);

void adc_init();
uint8_t adc_async_read(uint8_t pin, AdcAsyncReadCallback callback);
uint8_t adc_async_process();

#endif /* CQ_ADC_H_ */
