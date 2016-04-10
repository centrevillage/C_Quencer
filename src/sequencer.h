#include <avr/io.h>

#ifndef CQ_SEQUENCER_H_
#define CQ_SEQUENCER_H_

extern unsigned char active_seq[16];
extern volatile uint8_t current_step;
extern unsigned long bpsX1000;
extern unsigned char divide_count;
extern unsigned char current_pos;

void step_seq();

#endif /* CQ_SEQUENCER_H_ */

