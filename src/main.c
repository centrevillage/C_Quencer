#include <avr/io.h>
#include <string.h>

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

  // pin interrupt for PC and PD ports
  PCICR = _BV(PCIE1) | _BV(PCIE2);
  PCMSK1 = _BV(PCINT12) | _BV(PCINT13);
  PCMSK2 = _BV(PCINT21) | _BV(PCINT22) | _BV(PCINT23); 
}

// the setup function runs once when you press reset or power the board
void setup() {
  ACSR |= (1<<ACD); //analog comparator off

  // initialize pin
  setup_pins();
  reset_all_input();

  adc_init();
  spi_init();

  read_knob_values();
  update_knob_values();
}

static uint16_t prev_timer_count = 0xFFFF;
void loop() {
  cli();
  uint16_t current_timer_count = TCNT1;
  sei();
  output_led();
  if (prev_timer_count != current_timer_count) {
    output_osc_and_cv(current_timer_count);
    prev_timer_count = current_timer_count;
  }
}

// from Arduino ==
int main(void) {
  timer_init();
	setup();
  sei();
	for (;;) { loop(); }
	return 0;
}
