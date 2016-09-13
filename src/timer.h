#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef CQ_TIMER_H_
#define CQ_TIMER_H_

void timer_init();
unsigned long ticks();

extern volatile uint16_t current_wrap_count;

#endif /* CQ_TIMER_H_ */
