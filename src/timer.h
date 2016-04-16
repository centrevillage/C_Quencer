#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef CQ_TIMER_H_
#define CQ_TIMER_H_

void timer_init();
void start_gate_timer();
void update_step_time();
void start_seq();
void stop_seq();
unsigned long ticks();

extern volatile unsigned long current_wrap_count;

#endif /* CQ_TIMER_H_ */
