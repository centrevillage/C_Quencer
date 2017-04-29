#include <avr/io.h>

#ifndef CQ_VARIABLES_H_
#define CQ_VARIABLES_H_

extern volatile uint16_t active_seq_bits;
extern volatile uint8_t is_active_seq;
extern volatile uint8_t current_step;
extern volatile uint16_t step_interval;
extern volatile uint8_t divide_count;
extern volatile uint8_t active_step_gate;
extern volatile uint8_t current_pitch1;
extern volatile uint8_t current_pitch2;
extern volatile uint16_t current_pitch1_dec;
extern volatile uint16_t current_pitch2_dec;
extern volatile uint8_t slide_speed;
extern volatile uint8_t slide_type; // 0: normal slide, 1: buggy slide
extern volatile uint8_t selected_wavetable_type1;
extern volatile uint8_t selected_wavetable_type2;
extern volatile uint8_t selected_wavetable_type2_sign;
extern volatile uint8_t wave_phase_shift;
extern volatile uint8_t wave_phase_shift_cycle;
extern volatile uint8_t wave1_volume; // 1 = 12.5%, 8: 100%
extern volatile uint8_t wave2_volume; // 1 = 12.5%, 8: 100%
extern volatile uint8_t pitch_duration;
extern volatile uint8_t prev_pitch1;
extern volatile uint8_t prev_pitch2;
extern volatile uint16_t slide_pitch1;
extern volatile uint16_t slide_pitch2;
extern volatile uint32_t slide_buf_value1;
extern volatile uint32_t slide_buf_value2;

extern volatile uint8_t current_oct1;
extern volatile uint8_t current_note_num1;
extern volatile uint8_t current_oct2;
extern volatile uint8_t current_note_num2;

extern volatile uint8_t prev_oct1;
extern volatile uint8_t prev_note_num1;
extern volatile uint8_t prev_oct2;
extern volatile uint8_t prev_note_num2;

extern volatile uint8_t in_start_seq;

#endif /* CQ_VARIABLES_H_ */
