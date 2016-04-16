#include "timer.h"
#include "sequencer.h"
#include "input.h"

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
  TCCR2A = 0; // normal mode (overflow after about 16ms)
  TIMSK2 |= (1<<TOIE0); // overflow interrupt
}

void start_gate_timer() {
  //reset timer counter
  TCNT0 = 0; 
  //start timer
  TCCR0B |= (1<<CS02) | (1<<CS00); // divide 1024
  
  if (active_seq[current_step]) {
    //gate on
    PORTB &= ~_BV(2);
  }

  active_step_gate = 1;
}

void update_step_time() {
  if (current_values.v.swing > 0) {
    uint16_t offset_interval = ((long)step_interval * current_values.v.swing) / 255 / 2;
    if (current_step % 2 == 0) {
      OCR1A = step_interval + offset_interval;
    } else {
      OCR1A = step_interval - offset_interval;
    }
  } else {
    OCR1A = step_interval;
  }
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

volatile unsigned long current_wrap_count = 0L;
ISR (TIMER2_OVF_vect) {
  cli();
  ++current_wrap_count;
  sei();
}

// trig in when stop sequence
void start_trigger() {
  TCNT1 = 0;
  TCCR1B |= (1<<CS12); // divide 256
  OCR1A = 0xFFFF;
}

void start_seq() {
  TCNT1 = 0;
  TCCR1B |= (1<<CS12); // divide 256
}

void stop_seq() {
  TCCR1B &= ~(1<<CS12);
  TCNT1 = 0;
}

unsigned long ticks() {
  cli();
  unsigned long ticks = current_wrap_count * 256 + TCNT2;
  sei();
  return ticks;
}
