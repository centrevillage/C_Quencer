#include "variables.h"

volatile uint16_t active_seq_bits = 0;
volatile uint8_t is_active_seq = 0;
volatile uint8_t current_step = 0;
// 1 tick(timer1) = 16 usec, 120 BPM = 2sec / 16 usec / 16 = 7812.5 tick;
volatile uint16_t step_interval = 8000L;
volatile uint8_t  divide_count = 1;
volatile uint8_t active_step_gate = 0;
volatile uint8_t current_pitch1 = 0;
volatile uint8_t current_pitch2 = 0;
volatile uint16_t current_pitch1_dec = 0;
volatile uint16_t current_pitch2_dec = 0;
volatile uint8_t selected_wavetable_type1 = 0;
volatile uint8_t selected_wavetable_type2 = 0;
volatile uint8_t selected_wavetable_type2_sign = 1; // 1: ê≥, 0: ïâ
volatile uint8_t wave_phase_shift = 0;
volatile uint8_t wave_phase_shift_cycle = 0;
volatile uint8_t wave1_volume = 4;
volatile uint8_t wave2_volume = 4;
volatile uint8_t pitch_duration = 0;
volatile uint8_t slide_speed = 0;
volatile uint8_t current_oct1 = 0;
volatile uint8_t current_note_num1 = 0;
volatile uint8_t current_oct2 = 0;
volatile uint8_t current_note_num2 = 0;
volatile uint8_t prev_oct1 = 0;
volatile uint8_t prev_note_num1 = 0;
volatile uint8_t prev_oct2 = 0;
volatile uint8_t prev_note_num2 = 0;
volatile uint8_t prev_pitch1 = 255;
volatile uint8_t prev_pitch2 = 255;
volatile uint16_t slide_pitch1 = 0;
volatile uint16_t slide_pitch2 = 0;
volatile uint32_t slide_buf_value1 = 0;
volatile uint32_t slide_buf_value2 = 0;
volatile uint8_t slide_type = 0;
