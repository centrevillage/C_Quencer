#include "timer.h"
#include "input.h"
#include "sequencer.h"
#include "variables.h"
#include "dac.h"

// Timer setting
// timer0: gate length timer
// timer1: trigger step timer
// timer2: dac(cv&osc) timer
void timer_init() {
  // Timer 0 ==
  TCCR0A = 0; // normal mode (overflow after about 16ms)
  TIMSK0 |= (1<<TOIE0); // overflow interrupt

  // Timer 1 == 
  stop_seq();
  TCCR1B |= (1<<WGM12); // CTC
  update_step_time();
  TIMSK1 |= (1<<OCIE1A);

  // Timer 2 ==
  TCCR2A = 0; // normal mode (overflow after about 4ms)
  TIMSK2 |= (1<<TOIE0); // overflow interrupt
  TCCR2B |= (1<<CS02); // divide 256
}

// gate timer interrupt
ISR (TIMER0_OVF_vect) {
  //stop timer
  TCCR0B &= ~((1<<CS02) | (1<<CS00));
  //gate off(=HIGH) (PB2)
  PORTB |= _BV(2);
  active_step_gate = 0;
}

// step timer interrupt
ISR (TIMER1_COMPA_vect) {
  if (current_state.start) {
    step_seq();
    update_step_time();
  }
}

volatile uint16_t current_wrap_count = 0;
ISR (TIMER2_OVF_vect) {
  cli();
  ++current_wrap_count;
  sei();

  if (current_wrap_count % 4 == 0) {
    read_knob_values();
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
}

unsigned long ticks() {
  cli();
  uint32_t ticks = ((uint32_t)current_wrap_count * 256 + TCNT2) / 4;
  sei();
  return ticks;
}
