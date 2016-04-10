#include "sequencer.h"

unsigned char active_seq[16];
volatile uint8_t current_step = 0;
unsigned long bpsX1000 = 2000L;
unsigned char divide_count = 1;
unsigned char current_pos = 0;

void step_seq() {
  if (current_step < 15) {
    ++current_step;
  } else {
    current_step = 0;
  }
  read_knob_values();
  start_gate_timer();
}
