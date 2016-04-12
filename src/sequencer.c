#include "sequencer.h"
#include "input.h"
#include "timer.h"

volatile unsigned char active_seq[16];
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
  if (current_step < 15) {
    ++current_step;
  } else {
    current_step = 0;
  }
  read_knob_values();
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
