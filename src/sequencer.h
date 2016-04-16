#include <avr/io.h>
#include <stdlib.h>

#ifndef CQ_SEQUENCER_H_
#define CQ_SEQUENCER_H_

extern volatile unsigned char active_seq[16];
extern volatile uint8_t current_step;
extern volatile uint16_t step_interval;
extern volatile uint8_t divide_count;
extern volatile uint8_t current_pos;
extern volatile uint8_t active_step_gate;
extern volatile uint8_t current_pitch;

void step_seq();
void reset_seq();
void set_divide(uint8_t divide);
void set_step_interval(uint16_t tick);
void update_seq_pattern();
void randomize_seq();
void update_pitch();

extern uint8_t prev_pitch;

#endif /* CQ_SEQUENCER_H_ */

