#include "adc.h"

void adc_init() {
	uint8_t result;
 	// AVCC as ref voltage
	ADMUX =  (1<<REFS0) | (1<<ADLAR); // only use 8-bit (ADCH)
	ADCSRA = (1<<ADPS2) | (1<<ADPS0);  // 16MHz / 32 = 500 kHz
	ADCSRA |= (1<<ADEN);                  // ADC enable
 	// dummy readout
	ADCSRA |= (1<<ADSC);                  // single readout
	loop_until_bit_is_clear(ADCSRA, ADSC);
	result = ADCH;
}

uint8_t adc_read(uint8_t pin) {
  ADMUX = (ADMUX & 0xF0) | pin;
	ADCSRA |= (1<<ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);
	return ADCH;
};
