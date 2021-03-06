#include <avr/io.h>
#include <stdlib.h>

#ifndef CQ_SEQUENCER_H_
#define CQ_SEQUENCER_H_

void step_seq();
void reset_seq();
void set_divide(uint8_t divide);
void set_step_interval(uint16_t tick);
void update_seq_pattern();
void randomize_seq();
void update_pitch();
void update_oct_note();
void update_wave_shape();

uint16_t get_last_step_duration_ticks();

void start_gate_timer();
void update_step_time();
void start_trigger();
void start_seq();
void stop_seq();
void sync_clock();

#endif /* CQ_SEQUENCER_H_ */

