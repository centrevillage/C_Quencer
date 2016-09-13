#include <avr/io.h>
#include <string.h>

#include "timer.h"
#include "dac.h"
#include "adc.h"
#include "variables.h"
#include "sequencer.h"
#include "input.h"
#include "led.h"
#include "scale.h"
#include "eeprom.h"

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

  load_preset();

  read_knob_values();
  update_knob_values();
}

inline void update_knob_values_on_stop() {
  switch(edit_mode) {
    case NORMAL:
      if (!current_state.start && current_wrap_count % 64 == 0) {
        update_knob_values();
      }
      break;
    case SELECT:
      if (current_wrap_count % 64 == 0) {
        update_knob_values();
      }
      break;
    case SCALE:
    case PATTERN:
      break;
    default:
      break;
  }
}

static uint8_t prev_short_tick = 0xFF;
static uint8_t prev_timer_count = 0xFF;
void loop() {
  output_led();

  uint8_t current_timer_count = TCNT2;
  uint8_t short_tick = TCNT2 >> 2;
  uint8_t interval_count = current_timer_count - prev_timer_count;
  if (interval_count > 0) {
    output_osc_and_cv(interval_count, short_tick - prev_short_tick);
    prev_timer_count = current_timer_count;
    prev_short_tick = short_tick;

    update_knob_values_on_stop();
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
