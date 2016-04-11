#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/eeprom.h>

#include "timer.h"
#include "dac.h"
#include "adc.h"
#include "sequencer.h"
#include "input.h"
#include "led.h"

void setup_pins() {
  // 0-4: LED, 5-7: switch in
  DDRD = 0;
  // switch with pullup
  PORTD = 0b11100000;
  // OUTPUT: CS & LDAC & GATE & MOSI & SCK, INPUT: MISO
  DDRB = _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(5);
  // INPUT: POT1-4 & TRIG & REC SW & RESET
  DDRC = 0;
  // switch with pullup
  PORTC = _BV(5);
}

// the setup function runs once when you press reset or power the board
void setup() {
  ACSR |= (1<<ACD); //analog comparator off

  // initialize pin
  setup_pins();

  // initialize sequence
  memset(active_seq, 0, sizeof(active_seq));

  adc_init();
  spi_init();

  read_knob_values();
}

void loop() {
  output_led();
  output_osc();
}

// from Arduino ==
int main(void) {
  timer_init();
	setup();
  sei();
	for (;;) { loop(); }
	return 0;
}
