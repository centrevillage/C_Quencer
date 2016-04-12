#include "sequencer.h"
#include "input.h"
#include "timer.h"
#include "euclid.h"

volatile unsigned char active_seq[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t current_step = 0;
// 1 tick = 62.5usec,  2sec / 62.5usec / 16 = 2000L;
volatile uint16_t step_interval = 2000L;
volatile uint8_t  divide_count = 1;
volatile uint8_t  divide_idx = 1;

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
  start_gate_timer(); // TODO: アクティブなstepの場合だけゲートを出す
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
  if (current_values.step_rand > 0) {
    srand(button_history.last_tick);
    for (int i = 0; i < current_values.step_length; ++i) {
      if ((uint8_t)(rand() >> 8) < current_values.step_rand) {
        active_seq[i] = !active_seq[i];
      }
    }
  }
}
