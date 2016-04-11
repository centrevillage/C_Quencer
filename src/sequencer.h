#include <avr/io.h>

#ifndef CQ_SEQUENCER_H_
#define CQ_SEQUENCER_H_

extern volatile unsigned char active_seq[16];
extern volatile uint8_t current_step;
extern volatile uint16_t step_interval;
extern volatile uint8_t divide_count;
extern volatile uint8_t current_pos;

void step_seq();
void reset_seq();
void set_divide(uint8_t divide);
void set_step_interval(uint16_t tick);

#endif /* CQ_SEQUENCER_H_ */

