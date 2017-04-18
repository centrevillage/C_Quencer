#include "adc.h"

void adc_init() {
  uint8_t result;
  // AVCC as ref voltage
  ADMUX =  (1<<REFS0) | (1<<ADLAR); // only use 8-bit (ADCH)
  //ADCSRA = (1<<ADPS2) | (1<<ADPS0);  // 16MHz / 32 = 500 kHz
  //ADCSRA = (1<<ADPS2) | (1<<ADPS1);  // 16MHz / 64 = 250 kHz
  ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);  // 16MHz / 64 = 125 kHz
  ADCSRA |= (1<<ADEN);                  // ADC enable
  // dummy readout
  ADCSRA |= (1<<ADSC);                  // single readout
  loop_until_bit_is_clear(ADCSRA, ADSC);
  result = ADCH;
}

static volatile uint8_t adc_async_read_pin_index;
AdcAsyncReadCallback adc_async_read_callback;

uint8_t adc_async_read(uint8_t pin, AdcAsyncReadCallback callback) {
  if (!adc_async_read_callback) {
    adc_async_read_pin_index = pin;
    adc_async_read_callback = callback;
    ADMUX = (ADMUX & 0xF0) | pin;
    ADCSRA |= (1<<ADSC);
    return 1;
  }
  return 0;
}

uint8_t adc_async_process() {
  if (adc_async_read_callback && !(ADCSRA & _BV(ADSC))) {
    adc_async_read_callback(adc_async_read_pin_index, ADCH);
    adc_async_read_callback = 0;
    return 1;
  }
  return 0;
}
