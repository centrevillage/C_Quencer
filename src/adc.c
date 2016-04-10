#include "adc.h"

void adc_init() {
 	// AVCC as ref voltage
	ADMUX =  (1<<REFS0) | (1<<ADLAR); // only use 8-bit (ADCH)
	ADCSRA = (1<<ADPS2) |(1<<ADPS1)| (1<<ADPS0);     // adc prescaler (must be between 50 and 200kHz)
	ADCSRA |= (1<<ADEN);                  // ADC enable
 	// dummy readout
	ADCSRA |= (1<<ADSC);                  // single readout
	while (ADCSRA & (1<<ADSC) ) {}        // wait to finish
}

uint8_t adc_read(uint8_t pin) {
  ADMUX |= pin;
	ADCSRA |= (1<<ADSC);            // single readout
	while (ADCSRA & (1<<ADSC) ) {}  // wait to finish
	return ADCH;
};
