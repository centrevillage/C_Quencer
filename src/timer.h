#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef CQ_TIMER_H_
#define CQ_TIMER_H_

void timer_init();
extern volatile uint16_t current_wrap_count;

extern uint32_t hp_ticks();

inline uint16_t ticks() {
  return current_wrap_count;
}

#endif /* CQ_TIMER_H_ */
