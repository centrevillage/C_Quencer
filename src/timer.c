#include "timer.h"
#include "sequencer.h"
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
  TCCR1B |= (1<<CS12) | (1<<CS10); // divide 1024
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
  //gate on
  PORTB &= ~_BV(2);

}

// 16.000.000 / 1024 = 15625 = 1 sec
// bpsX1000 in (239 (=14BPM) upto 15625(=937.5 BPM)
void update_step_time() {
  OCR1A = (uint16_t) 15625L * 1000L / bpsX1000 / divide_count;
}

// gate timer interrupt
ISR (TIMER0_OVF_vect) {
  //stop timer
  TCCR0B &= ~((1<<CS02) | (1<<CS00));
  //gate off(=HIGH) (PB2)
  PORTB |= _BV(2);
}

// step timer interrupt
ISR (TIMER1_COMPA_vect) {
  step_seq();
}

// DAC interrupt
ISR (TIMER2_OVF_vect) {
  output_osc();
}
