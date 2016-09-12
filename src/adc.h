#include <avr/io.h>

#ifndef CQ_ADC_H_
#define CQ_ADC_H_

void adc_init();

inline uint8_t adc_read(uint8_t pin) {
  ADMUX = (ADMUX & 0xF0) | pin;
  ADCSRA |= (1<<ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);
  return ADCH;
}

#endif /* CQ_ADC_H_ */
