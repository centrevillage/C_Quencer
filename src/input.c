#include "input.h"
#include "sequencer.h"
#include "adc.h"

// input task
void read_knob_values() {
  for (int i = 0; i < 4; ++i) {
    uint8_t new_value = adc_read(i);
    if (new_value != knob_values[i]) {
      set_current_value(new_value, i); 
      knob_values[i] = adc_read(i);
    }
  } 
}

void set_current_value(uint8_t value, uint8_t knob_idx) {
  switch (knob_idx) {
    case 0: // fill / len / glide
      if (func_on()) {
        current_values.step_fill = value & 0xF0 + 1;
      } else if (hid_on()) {
        current_values.glide = value;
      } else {
        current_values.step_length = value & 0xF0 + 1;
      }
      break;
    case 1: // rot / rand / swing
      if (func_on()) {
        current_values.step_rand = value;
      } else if (hid_on()) {
        current_values.swing = value;
      } else {
        current_values.step_rot = value & 0xF0;
      }
      break;
    case 2: // scale select / transpose / scale pattern random
      if (func_on()) {
        current_values.scale_transpose = value;
      } else if (hid_on()) {
        current_values.scale_select_random = value;
      } else {
        current_values.scale_select = value ;
      }
      break;
    case 3: // scale pattern / scale range / scale pattern random
      if (func_on()) {
        current_values.scale_range = value;
      } else if (hid_on()) {
        current_values.scale_pattern_random = value;
      } else {
        current_values.scale_select = value ;
      }
      break;
    default:
      break;
  }
}

uint8_t func_on() {
  return current_state.bit.func_lock ^ current_state.bit.func;
}

uint8_t hid_on() {
  return current_state.bit.hid;
}

// button intrrupt

// FUNC BUTTON = PD5, PCINT21
// START BUTTON = PD6, PCINT22
// TAP BUTTON = PD7, PCINT23
ISR(PCINT2_vect) {
  if (bit_is_clear(PIND,5)) {
    if (!button_state[0]) {
      // do something
      button_state[0] = 1;
    } else {
      button_state[0] = 0;
    }
  }	
  if (bit_is_clear(PIND,6)) {
    if (!button_state[1]) {
      // do something
      button_state[1] = 1;
    } else {
      button_state[1] = 0;
    }
  }	
  if (bit_is_clear(PIND,7)) {
    // do something
    if (!button_state[2]) {
      // do something
      button_state[2] = 1;
    } else {
      button_state[2] = 0;
    }
  }	
  return;	
};

// TING IN = PC4, PCINT12
// REC BUTTON = PC5, PCINT13
ISR(PCINT1_vect) {
  if(bit_is_clear(PINC,4)) {
    step_seq();
  }	
  if(bit_is_clear(PINC,5)) {
    if (!button_state[3]) {
      button_state[3] = 1;
      // rec
    }
  } else {
    button_state[3] = 0;
  }
  return;	
};

