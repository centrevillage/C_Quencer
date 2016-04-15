#include "sequencer.h"
#include "input.h"
#include "timer.h"
#include "euclid.h"
#include "pattern.h"

volatile unsigned char active_seq[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t current_step = 0;
// 1 tick = 62.5usec,  2sec / 62.5usec / 16 = 2000L;
volatile uint16_t step_interval = 2000L;
volatile uint8_t  divide_count = 1;
volatile uint8_t  divide_idx = 1;
volatile uint8_t active_step_gate = 0;
volatile uint8_t current_pitch = 0;

void step_seq() {
  if (divide_idx < divide_count) {
    ++divide_idx;
    return;
  }
  divide_idx = 1;
  if (current_step < (current_values.step_length-1)) {
    ++current_step;
  } else {
    current_step = 0;
    update_seq_pattern();
  }
  read_knob_values();
  update_pitch();
  start_gate_timer();
}

void reset_seq() {
  current_step = 16; // next -> 0
}

void set_divide(uint8_t divide) {
  if (divide_count != divide) {
    divide_count = divide;
    divide_idx = 1;
  }
}

void set_step_interval(uint16_t tick) {
  step_interval = tick;
}

void update_seq_pattern() {
  uint16_t euclid_seq = euclid_seq_table[current_values.step_length-1][current_values.step_fill-1];
  for (int i = current_values.step_rot % current_values.step_length, j = 0; j < current_values.step_length; i = (i+1) % current_values.step_length, ++j) {
    active_seq[j] = euclid_seq & (1<<i);
  }
  randomize_seq();
}

void randomize_seq() {
  srand(button_history.last_tick);
  if (current_values.step_rand > 0) {
    for (int i = 0; i < current_values.step_length; ++i) {
      if ((uint8_t)(rand() >> 8) < current_values.step_rand) {
        active_seq[i] = !active_seq[i];
      }
    }
  }
}

//CDEFF#GAB = 0 2 4 5 6 7 9 11 
static uint8_t pattern_idx_to_pitch_idx[8] = {0, 2, 4, 5, 6, 7, 9, 11};

void update_pitch() {
  long pattern_value = (scale_patterns[current_values.scale_pattern][current_step] - 8) * current_values.scale_range;
  long rand_value = (uint8_t)(((int)current_values.scale_pattern_random * (int)(((rand() & 0xFF00) >> 8) - 127))/16);
  uint16_t tmp_value = (pattern_value + rand_value) / 128;

  // quantize
  uint16_t base_value = tmp_value / 8 * 8;
  int upper_value = tmp_value - base_value;
  uint8_t near_minus_idx = 12;
  uint8_t near_plus_idx = 12;
  for (int i = upper_value; i < 8; ++i) {
    if (current_values.scale_select & (1<<i)) {
      near_plus_idx = i;
      break;
    }
  }
  for (int i = upper_value; i >= 0; --i) {
    if (current_values.scale_select & (1<<i)) {
      near_minus_idx = i;
      break;
    }
  }
  if (near_minus_idx > near_plus_idx) {
    tmp_value = base_value + upper_value + near_plus_idx;
  } else {
    tmp_value = base_value + upper_value - near_minus_idx;
  }
  base_value = tmp_value / 8 * 8;
  upper_value = tmp_value - base_value;

  uint8_t current_pitch_tmp = (base_value/8*12) + pattern_idx_to_pitch_idx[upper_value] + current_values.scale_transpose;
  if (current_pitch_tmp > 95) {
    current_pitch = 95;
  } else {
    current_pitch = current_pitch_tmp;
  }
}
