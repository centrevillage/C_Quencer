#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef CQ_TIMER_H_
#define CQ_TIMER_H_

void timer_init();
void start_gate_timer();
void update_step_time();

#endif /* CQ_TIMER_H_ */
