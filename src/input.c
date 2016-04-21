#include "input.h"
#include "sequencer.h"
#include "adc.h"
#include "timer.h"
#include "led.h"

volatile ControllerValue current_values;
volatile static ControllerValue operate_values;

volatile enum RecMode  rec_mode = STOP;
volatile enum FuncMode func_mode = NONE;

volatile ControllerState current_state;
volatile ButtonHistory button_history;

volatile static uint8_t current_knob_value_idx = 0;

// input task
void read_knob_values() {
  uint16_t prev_value;
  uint16_t new_value;

  for (uint8_t i = 0; i < 4; ++i) {
    prev_value = 0;
    for (uint8_t j = 0; j < 8; ++j) {
      prev_value += knob_values[i][j];
    }
    prev_value = prev_value / 8;
    new_value = 0;
    knob_values[i][current_knob_value_idx] = 255 - adc_read(i);
    for (uint8_t j = 0; j < 8; ++j) {
      new_value += knob_values[i][j];
    }
    new_value = new_value / 8;
    if (new_value != prev_value) {
      set_current_value((uint8_t)new_value, i); 
    }
  } 
  ++current_knob_value_idx;
  if (current_knob_value_idx >= 8) {
    current_knob_value_idx = 0;
  }
}

uint8_t is_multi_tap(uint8_t button_idx, uint8_t count) {
  return button_history.button_idx == button_idx && button_history.count >= count;
}

void set_current_value(uint8_t value, uint8_t knob_idx) {
  // reset tap button history
  if (is_multi_tap(2, 1)) {
    button_history.count = 0;
  }
  uint8_t is_change_seq = 0;
  switch (knob_idx) {
    case 0: // fill / len / slide
      switch (func_mode) {
        case NONE:
          operate_values.v.step_fill = ((value & 0xF0) >> 4);
          changed_value_flags.v.step_fill = 1;
          is_change_seq = 1;
          break;
        case FUNC:
          operate_values.v.step_length = ((value & 0xF0) >> 4) + 1;
          changed_value_flags.v.step_length = 1;
          is_change_seq = 1;
          break;
        case HID:
          operate_values.v.slide = value;
          changed_value_flags.v.slide = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        default:
          break;
      }
      break;
    case 1: // rot / rand / swing
      switch (func_mode) {
        case NONE:
          operate_values.v.step_rot = (value & 0xF0) >> 4;
          changed_value_flags.v.step_rot = 1;
          is_change_seq = 1;
          break;
        case FUNC:
          operate_values.v.step_rand = value / 2;
          changed_value_flags.v.step_rand = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case HID:
          operate_values.v.swing = value;
          changed_value_flags.v.swing= 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    case 2: // scale select / transpose / scale pattern random
      switch (func_mode) {
        case NONE:
          operate_values.v.scale_select = (value & 0xF0) >> 4;
          changed_value_flags.v.scale_select = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case FUNC:
          operate_values.v.scale_transpose = (uint16_t)value * 76 / 256;
          changed_value_flags.v.scale_transpose = 1;
          set_display_mode(TRANSPOSE);
          break;
        case HID:
          operate_values.v.scale_shift = value / 4;
          changed_value_flags.v.scale_shift = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    case 3: // scale pattern / scale range / scale pattern random
      switch (func_mode) {
        case NONE:
          operate_values.v.scale_pattern = (value & 0xF0) >> 4;
          changed_value_flags.v.scale_pattern = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case FUNC:
          operate_values.v.scale_range = ((value & 0xF0) >> 4) + 1;
          changed_value_flags.v.scale_range = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case HID:
          operate_values.v.scale_pattern_random = (value & 0xF0) >> 4;
          changed_value_flags.v.scale_pattern_random = 1;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    default:
      break;
  }
  current_values = operate_values;
  if (is_change_seq) {
    set_display_mode(SEQ);
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
    button_history.count = 1;
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

void update_button_history_on_leave(uint8_t button_idx) {
  enum FuncMode mode = get_func_mode();
  if (button_history.mode == mode && button_history.button_idx == button_idx) {
    unsigned long t = ticks();
    button_history.last_leave = t;
  } else {
    button_history.last_leave = 0;
  }
};

void press(uint8_t button_idx) {
  ButtonHistory prev_button_history = button_history;

  update_button_history(button_idx);

  if (is_multi_tap(button_idx, 2)) {
    if (button_history.interval_tick < 200) {
      // avoid fluttering
      button_history = prev_button_history;
      return;
    } else if (button_history.interval_tick > 262143L) {
      // timeout multitap
      reset_button_history(button_idx);
    }
  }

  switch(button_idx) {
    case 0:
      current_state.func = 1;
      break;
    case 1:
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
      break;
    case 2:
      if (func_mode == FUNC) {
        if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 65535) {
          set_divide(button_history.count);
        } else {
          set_divide(1);
          reset_button_history(button_idx);
        }
      } else {
        current_state.hid = 1;
        if (is_multi_tap(button_idx, 2) && ((button_history.last_leave < button_history.last_tick) || (button_history.last_leave - button_history.last_tick) > 4096)) {
          set_step_interval(button_history.interval_tick/8);
        }
      }
      break;
    case 3:
      if (func_mode == FUNC) {
        switch (rec_mode) {
          case STOP:
            rec_mode = PLAY;
            set_display_mode(REC_REDO);
            break;
          case PLAY:
            rec_mode = STOP;
            set_display_mode(REC_CLEAR);
            break;
          case REC:
            clear_recording();
            set_display_mode(REC_CLEAR);
            break;
          default:
            break;
        }
      } else if (func_mode == NONE) {
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
      break;
    default:
      break;
  }
  func_mode = get_func_mode();
}

void leave(uint8_t button_idx) {
  update_button_history_on_leave(button_idx);
  switch(button_idx) {
    case 0:
      current_state.func = 0;
      break;
    case 1:
      break;
    case 2:
      current_state.hid = 0;
      break;
    case 3:
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
    if (!current_state.start) {
      start_trigger();
    }
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
static volatile uint8_t play_pos = 0;

void next_record_pos() {
  ++record_pos;
  if (record_pos >= 64) {
    record_pos = 0;
  }
}

void next_play_pos() {
  ++play_pos;
  if (play_pos >= record_length) {
    play_pos = 0;
  }
  record_pos = (play_pos + record_start) % 64;
}

void record_current_knob_values() {
  for (int i = 0; i < sizeof(ControllerValue); ++i) {
    if (changed_value_flags.values[i]) {
      recorded_values[record_pos].values[i] = operate_values.values[i];
      recorded_value_flags[record_pos].values[i] = 1;
    }
  }

  if (record_length < 64) {
    record_length++;  
  }
}

void start_recording() {
  record_start = record_pos;
  record_length = 0;  
}

void end_recording() {
  record_end = record_pos;
  int record_start_tmp = ((int)record_end) - ((int)record_length);
  record_start = (uint8_t)(record_start_tmp < 0 ? (record_start_tmp + 64) : record_start_tmp);
  play_pos = 0;
}

void play_recorded_knob_values() {
  for (int i = 0; i < sizeof(ControllerValue); ++i) {
    if (!changed_value_flags.values[i] && recorded_value_flags[record_pos].values[i]) {
      current_values.values[i] = recorded_values[record_pos].values[i];
    }
  }
}

void clear_recording() {
  memset(&recorded_values, 0, sizeof(ControllerValue) * 64);
  memset(&recorded_value_flags, 0, sizeof(ControllerValue) * 64);
  current_values = operate_values;
}

void reset_all_input() {
  current_values.v.step_fill = 4;
  current_values.v.step_length = 16;
  current_values.v.step_rot = 0;
  current_values.v.step_rand = 0;
  current_values.v.scale_select = 0;
  current_values.v.scale_shift = 64;
  current_values.v.scale_pattern = 0;
  current_values.v.scale_transpose = 36;
  current_values.v.scale_range = 8;
  current_values.v.scale_pattern_random = 0;
  current_values.v.slide = 0;
  current_values.v.swing = 0;

  operate_values = current_values;

  record_start = 0;
  record_end = 0;
  record_length = 0;
  record_pos = 0;
  rec_mode = STOP;
  func_mode = NONE;
  button_history.mode = NONE;
  button_history.button_idx = 5;
  memset(knob_values, 0, 4*8);
  memset(button_state, 0, 4);
  memset(&changed_value_flags, 0, sizeof(ControllerValue));
  memset(&recorded_values, 0, sizeof(ControllerValue) * 64);
  memset(&recorded_value_flags, 0, sizeof(ControllerValue) * 64);
}
