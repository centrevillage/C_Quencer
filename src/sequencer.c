#include "sequencer.h"
#include "input.h"
#include "timer.h"
#include "euclid.h"
#include "pattern.h"

volatile unsigned char active_seq[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t current_step = 0;
// 1 tick(timer1) = 16 usec, 120 BPM = 2sec / 16 usec / 16 = 7812.5 tick;
volatile uint16_t step_interval = 8000L;
volatile uint8_t  divide_count = 1;
volatile uint8_t  divide_idx = 1;
volatile uint8_t active_step_gate = 0;
volatile uint8_t current_pitch = 0;

void step_seq() {
  if (divide_idx < divide_count) {
    ++divide_idx;
    return;
  }
  cli();
  divide_idx = 1;
  if (current_step < (current_values.v.step_length-1)) {
    ++current_step;
  } else {
    current_step = 0;
    update_seq_pattern();
  }
  read_knob_values();
  if (rec_mode == PLAY || rec_mode == REC) {
    play_recorded_knob_values();
    if (rec_mode == REC) {
      record_current_knob_values();
    }
  }
  if (active_seq[current_step]) {
    update_pitch();
  }
  start_gate_timer();

  memset((ControllerValue*)&changed_value_flags, 0, sizeof(ControllerValue));
  sei();
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
  uint16_t euclid_seq = euclid_seq_table[current_values.v.step_length-1][current_values.v.step_fill];
  for (int i = current_values.v.step_rot % current_values.v.step_length, j = 0; j < current_values.v.step_length; i = (i+1) % current_values.v.step_length, ++j) {
    active_seq[j] = euclid_seq & (1<<i);
  }
  randomize_seq();
}

void randomize_seq() {
  srand(button_history.last_tick);
  if (current_values.v.step_rand > 0) {
    for (int i = 0; i < current_values.v.step_length; ++i) {
      if ((uint8_t)(rand() >> 8) < current_values.v.step_rand) {
        active_seq[i] = !active_seq[i];
      }
    }
  }
}

//CDEFF#GAB = 0 2 4 5 6 7 9 11 
static const uint8_t pattern_idx_to_pitch_idx[8] = {0, 2, 4, 5, 6, 7, 9, 11};

uint8_t prev_pitch = 255;
void update_pitch() {
  prev_pitch = current_pitch;
  long pattern_value = (scale_patterns[current_values.v.scale_pattern][current_step] - 8) * current_values.v.scale_range;
  long rand_value = (uint8_t)(((int)current_values.v.scale_pattern_random * (int)((rand() >> 8) - 127))/16);
  long tmp_value_long = (pattern_value + rand_value);
  if (tmp_value_long < 0) {
    tmp_value_long = 0;
  }
  int tmp_value = ((uint16_t)tmp_value_long) / 128 + current_values.v.scale_shift;
  if (tmp_value < 0) {
    tmp_value = 0;
  }

  // quantize
  uint16_t base_value = tmp_value / 8 * 8;
  int upper_value = tmp_value - base_value;
  for (int i = 0; i < 8; ++i) {
    if (current_values.v.scale_select & (1<<((upper_value+i)%8))) {
      tmp_value += i;
      break;
    }
    if (current_values.v.scale_select & (1<<((upper_value-i)%8))) {
      tmp_value -= i;
      break;
    }
  }
  base_value = tmp_value / 8 * 8;
  upper_value = tmp_value - base_value;

  int current_pitch_tmp = (base_value/8*12) + pattern_idx_to_pitch_idx[upper_value] + (current_values.v.scale_transpose - 36);
  if (current_pitch_tmp > 119) {
    current_pitch = 119;
  } else if (current_pitch_tmp < 0) {
    current_pitch = 0;
  } else {
    current_pitch = current_pitch_tmp;
  }
}
