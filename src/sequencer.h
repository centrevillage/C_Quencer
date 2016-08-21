#include <avr/io.h>
#include <stdlib.h>

#ifndef CQ_SEQUENCER_H_
#define CQ_SEQUENCER_H_

void step_seq();
void step_seq_on_normal();
void step_seq_on_edit_scale();
void step_seq_on_edit_pattern();
void reset_seq();
void set_divide(uint8_t divide);
void set_step_interval(uint16_t tick);
void update_seq_pattern();
void randomize_seq();
void update_pitch();
void update_oct_note();
void update_slide();
void update_wave_shape();

void start_gate_timer();
void update_step_time();
void start_trigger();
void start_seq();
void stop_seq();

#endif /* CQ_SEQUENCER_H_ */

