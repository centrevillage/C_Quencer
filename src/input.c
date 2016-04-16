#include "input.h"
#include "sequencer.h"
#include "adc.h"
#include "timer.h"
#include "led.h"

volatile ControllerValue current_values;

volatile enum RecMode  rec_mode = STOP;
volatile enum FuncMode func_mode = NONE;

volatile static union ControllerState current_state;
volatile struct ButtonHistory button_history;

// input task
void read_knob_values() {
  for (int i = 0; i < 4; ++i) {
    uint8_t new_value = adc_read(i);
    if (new_value != knob_values[i]) {
      set_current_value(new_value, i); 
      knob_values[i] = new_value;
    }
  } 
}

void set_current_value(uint8_t value, uint8_t knob_idx) {
  uint8_t is_change_seq = 0;
  switch (knob_idx) {
    case 0: // fill / len / glide
      switch (func_mode) {
        case NONE:
          current_values.v.step_fill = ((value & 0xF0) >> 4);
          changed_value_flags.v.step_fill = 1;
          is_change_seq = 1;
          break;
        case FUNC:
          current_values.v.step_length = ((value & 0xF0) >> 4) + 1;
          changed_value_flags.v.step_length = 1;
          is_change_seq = 1;
          break;
        case HID:
          current_values.v.glide = value;
          changed_value_flags.v.glide = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        default:
          break;
      }
      break;
    case 1: // rot / rand / swing
      switch (func_mode) {
        case NONE:
          current_values.v.step_rot = (value & 0xF0) >> 4;
          changed_value_flags.v.step_rot = 1;
          is_change_seq = 1;
          break;
        case FUNC:
          current_values.v.step_rand = value / 2;
          changed_value_flags.v.step_rand = 1;
          is_change_seq = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case HID:
          current_values.v.swing = value;
          changed_value_flags.v.swing= 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    case 2: // scale select / transpose / scale pattern random
      switch (func_mode) {
        case NONE:
          current_values.v.scale_select = value;
          changed_value_flags.v.scale_select = 1;
          set_display_mode(SCALE);
          break;
        case FUNC:
          current_values.v.scale_transpose = (uint16_t)value * 76 / 256;
          changed_value_flags.v.scale_transpose = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case HID:
          current_values.v.scale_shift = value / 4;
          changed_value_flags.v.scale_shift = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    case 3: // scale pattern / scale range / scale pattern random
      switch (func_mode) {
        case NONE:
          current_values.v.scale_pattern = (value & 0xF0) >> 4;
          changed_value_flags.v.scale_pattern = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case FUNC:
          current_values.v.scale_range = value;
          changed_value_flags.v.scale_range = 1;
          set_led_count((value & 0xF0 >> 4) + 1);
          break;
        case HID:
          current_values.v.scale_pattern_random = value;
          changed_value_flags.v.scale_pattern_random = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    default:
      break;
  }
  if (is_change_seq) {
    update_seq_pattern();
  }
}

enum FuncMode get_func_mode() {
  if (current_state.hid) {
    return HID;
  } else if (current_state.func_lock ^ current_state.func) {
    return FUNC;
  }
  return NONE;
}

void reset_button_history(uint8_t button_idx) {
    unsigned long t = ticks();
    enum FuncMode mode = get_func_mode();
    button_history.mode = mode;
    button_history.button_idx = button_idx;
    button_history.count = 0;
    button_history.last_tick = t;
    button_history.interval_tick = 0;
}

void update_button_history(uint8_t button_idx) {
  enum FuncMode mode = get_func_mode();
  if (button_history.mode == mode && button_history.button_idx == button_idx) {
    unsigned long t = ticks();
    button_history.button_idx = button_idx;
    button_history.count += 1;
    button_history.interval_tick = t - button_history.last_tick;
    button_history.last_tick = t;
  } else {
    reset_button_history(button_idx);
  }
};

uint8_t is_multi_tap(uint8_t button_idx, uint8_t count) {
  return button_history.button_idx == button_idx && button_history.count >= count;
}

void press(uint8_t button_idx) {
  struct ButtonHistory prev_button_history = button_history;
  update_button_history(button_idx);

  switch(button_idx) {
    case 0:
      if (is_multi_tap(button_idx, 2)) {
        if (button_history.interval_tick < 8000) { // 8000 tick = 500 msec
          // double tap = func lock
          if (button_history.interval_tick < 100) { // 100 tick = 6.25 msec
            // avoid fluttering
            current_state.func = 1;
          } else {
            current_state.func_lock = !current_state.func_lock;
          }
        } else {
          current_state.func = 1;
        }
        reset_button_history(button_idx);
      } else {
        current_state.func = 1;
      }
      break;
    case 1:
      if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 100) {
        // avoid fluttering
        reset_button_history(button_idx);
      } else {
        if (func_mode == FUNC) {
          reset_seq();
        } else {
          if (current_state.start) {
            current_state.start = 0;
            stop_seq();
          } else {
            current_state.start = 1;
            start_seq();
          }
        } 
      }
      break;
    case 2:
      if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 100) {
        // avoid fluttering
        button_history = prev_button_history;
      } else {
          if (func_mode == FUNC) {
            if (button_history.interval_tick < 16000) { // 16000 tick = 1000 msec
              set_divide(button_history.count);
            } else {
              reset_button_history(button_idx);
              set_divide(1);
            }
          } else {
            if (is_multi_tap(button_idx, 2)) {
              set_step_interval(button_history.interval_tick);
            }
          }
      }
      break;
    case 3:
      if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 100) {
        // avoid fluttering
        reset_button_history(button_idx);
      } else {
        if (button_state[2]) {
          // TAP+REC = hidden mode
          current_state.hid = 1;
        } else {
          if (func_mode == FUNC) {
            switch (rec_mode) {
              case STOP:
                rec_mode = PLAY;
                break;
              case PLAY:
                rec_mode = STOP;
                break;
              case REC:
                clear_recording();
                break;
              default:
                break;
            }
          } else {
            if (!current_state.rec) {
              if (rec_mode == STOP) {
                clear_recording();
              }
              rec_mode = REC;
              start_recording();
            } else {
              rec_mode = PLAY;
              end_recording();
            }
            current_state.rec = !current_state.rec;
          }
        }
        break;
      }
    default:
      break;
  }
  func_mode = get_func_mode();
}

void leave(uint8_t button_idx) {
  switch(button_idx) {
    case 0:
      current_state.func = 0;
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      current_state.hid = 0;
      break;
    default:
      break;
  }
  func_mode = get_func_mode();
}

// button intrrupt

// FUNC BUTTON = PD5, PCINT21
// START BUTTON = PD6, PCINT22
// TAP BUTTON = PD7, PCINT23
ISR(PCINT2_vect) {
  for (int i = 0; i < 3; ++i) {
    int pin = i + 5;
    if (bit_is_clear(PIND, pin)) {
      if (!button_state[i]) {
        press(i);
      }
      button_state[i] = 1;
    } else {
      if (button_state[i]) {
        leave(i);
      }
      button_state[i] = 0;
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
  if (bit_is_clear(PINC, 5)) {
    if (!button_state[3]) {
      press(3);
    }
    button_state[3] = 1;
  } else {
    if (button_state[3]) {
      leave(3);
    }
    button_state[3] = 0;
  }
  return;	
};


static volatile uint8_t record_start = 0;
static volatile uint8_t record_end = 0;
static volatile uint8_t record_length = 0;
static volatile uint8_t record_pos = 0;
void record_current_knob_values() {
  for (int i = 0; i < sizeof(ControllerValue); ++i) {
    if (changed_value_flags.values[i]) {
      recorded_values[record_pos].values[i] = current_values.values[i];
      recorded_value_flags[record_pos].values[i] = 1;
    }
  }

  record_pos++;
  if (record_pos >= 128) {
    record_pos = 0;
  }

  if (record_length < 128) {
    record_length++;  
  }
}

void start_recording() {
  record_start = record_pos;
  record_length = 0;  
}

void end_recording() {
  record_end = record_pos;
  int record_start_tmp = record_end - record_length;
  record_start = (uint8_t)(record_start_tmp < 0 ? (record_start_tmp + 128) : record_start_tmp);
}

void play_recorded_knob_values() {
  for (int i = 0; i < sizeof(ControllerValue); ++i) {
    if (!changed_value_flags.values[i] && recorded_value_flags[record_pos].values[i]) {
      current_values.values[i] = recorded_values[record_pos].values[i];
    }
  }
}

void clear_recording() {
  memset(&recorded_values, 0, sizeof(ControllerValue) * 128);
  memset(&recorded_value_flags, 0, sizeof(ControllerValue) * 128);
}
