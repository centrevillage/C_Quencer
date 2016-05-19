#include "input.h"
#include "sequencer.h"
#include "adc.h"
#include "timer.h"
#include "led.h"
#include "eeprom.h"

volatile ControllerValue current_values;

volatile enum EditMode edit_mode = NORMAL;
volatile enum RecMode  rec_mode = STOP;
volatile enum FuncMode func_mode = NONE;

volatile ControllerState current_state;
volatile ButtonHistory button_history;
volatile KnobHistory knob_history;

volatile uint8_t edit_preset_num;
volatile uint8_t edit_pos;
volatile uint16_t edit_scale;
volatile uint8_t edit_pattern[16];

volatile static uint8_t current_knob_idx = 0;
volatile static uint8_t current_knob_value_idx = 0;


// input task
void read_knob_values() {
  knob_values[current_knob_idx][current_knob_value_idx] = 255 - adc_read(current_knob_idx);
  ++current_knob_idx;
  if (current_knob_idx > 3) {
    current_knob_idx = 0;
    ++current_knob_value_idx;
    if (current_knob_value_idx >= KNOB_VALUES_SIZE) {
      current_knob_value_idx = 0;
    }
  }
}

static volatile uint16_t prev_values[] = {0, 0, 0, 0};
void update_knob_values() {
  uint16_t prev_value;
  uint16_t new_value;
  for (uint8_t i = 0; i < 4; ++i) {
    prev_value = prev_values[i];
    new_value = 0;
    for (uint8_t j = 0; j < KNOB_VALUES_SIZE; ++j) {
      new_value += knob_values[i][j];
    }
    new_value = new_value / KNOB_VALUES_SIZE;
    if (new_value != prev_value) {
      set_current_value((uint8_t)new_value, i); 
    }
    prev_values[i] = new_value;
  } 
}

void reset_knob_history(uint8_t knob_idx) {
  unsigned long t = ticks();
  knob_history.knob_idx = knob_idx;
  knob_history.count = 1;
  knob_history.last_tick = t;
  knob_history.interval_tick = 0;
}

void update_knob_history(uint8_t knob_idx) {
  if (knob_history.knob_idx == knob_idx) {
    unsigned long t = ticks();
    knob_history.knob_idx = knob_idx;
    knob_history.count += 1;
    knob_history.interval_tick = t - knob_history.last_tick;
    knob_history.last_tick = t;
  } else {
    reset_knob_history(knob_idx);
  }
};

uint8_t is_multi_tap(uint8_t button_idx, uint8_t count) {
  return button_history.button_idx == button_idx && button_history.count >= count;
}

uint8_t is_knob_multi(uint8_t knob_idx, uint8_t count) {
  return knob_history.knob_idx == knob_idx && knob_history.count >= count;
}

void set_current_value(uint8_t value, uint8_t knob_idx) {
  // reset tap button history
  if (is_multi_tap(2, 1)) {
    button_history.count = 0;
  }

  switch(edit_mode) {
    case NORMAL:
      set_current_value_on_normal(value, knob_idx);
      break;
    case SELECT:
      set_current_value_on_select(value, knob_idx);
      break;
    case SCALE:
      set_current_value_on_scale(value, knob_idx);
      break;
    case PATTERN:
      set_current_value_on_pattern(value, knob_idx);
      break;
    default:
      break;
  }
}

void set_current_value_on_normal(uint8_t value, uint8_t knob_idx){
  uint8_t is_change_seq = 0;
  switch (knob_idx) {
    case 0: // fill / len / slide
      switch (func_mode) {
        case NONE:
          current_values.v.step_fill = ((value & 0xF0) >> 4);
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_FILL;
          is_change_seq = 1;
          break;
        case FUNC:
          current_values.v.step_length = ((value & 0xF0) >> 4) + 1;
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_LENGTH;
          is_change_seq = 1;
          break;
        case HID:
          current_values.v.slide = value / 2;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SLIDE;
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
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_ROT;
          is_change_seq = 1;
          break;
        case FUNC:
          current_values.v.step_rand = value / 2;
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_RAND;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case HID:
          current_values.v.swing = value / 2;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SWING;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    case 2: // scale select / transpose / scale pattern random
      switch (func_mode) {
        case NONE:
          current_values.v.scale_select = (value & 0xF0) >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_SELECT;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case FUNC:
          current_values.v.scale_transpose = (uint16_t)value * 76 / 256;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_TRANSPOSE;
          set_display_mode(TRANSPOSE);
          break;
        case HID:
          current_values.v.scale_shift = value / 4 + 16;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_SHIFT;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    case 3: // scale pattern / scale range / scale pattern random
      switch (func_mode) {
        case NONE:
          current_values.v.scale_pattern = (value & 0xF0) >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_PATTERN;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case FUNC:
          current_values.v.scale_range = ((value & 0xF0) >> 4) + 1;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_RANGE;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
        case HID:
          current_values.v.scale_pattern_random = (value & 0xF0) >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_PAT_RAND;
          set_led_count(((value & 0xF0) >> 4) + 1);
          break;
      }
      break;
    default:
      break;
  }
  if (is_change_seq) {
    set_display_mode(SEQ);
    update_seq_pattern();
  }
}

void set_current_value_on_select(uint8_t value, uint8_t knob_idx){
  if (knob_idx == 2){
    update_knob_history(knob_idx);
    if (is_knob_multi(2, 3)) {
      // select scale edit
      enter_edit_scale_mode();
      reset_knob_history(knob_idx);
    }
  } else if (knob_idx == 3) {
    update_knob_history(knob_idx);
    if (is_knob_multi(3, 3)) {
      // select pattern edit
      enter_edit_pattern_mode();
      reset_knob_history(knob_idx);
    }
  }

  if (knob_idx == 2 && is_knob_multi(2, 4)) {
    // select scale edit
    enter_edit_scale_mode();
    reset_knob_history(knob_idx);
    set_current_value_on_scale(value, knob_idx);
  } else if (is_knob_multi(3, 4)) {
    // select pattern edit
    enter_edit_pattern_mode();
    reset_knob_history(knob_idx);
    set_current_value_on_pattern(value, knob_idx);
  }
}

void set_current_value_on_scale(uint8_t value, uint8_t knob_idx){
  update_knob_history(knob_idx);

  if (is_knob_multi(3, 4)) {
    // select pattern edit
    enter_edit_pattern_mode();
    reset_knob_history(knob_idx);
    return;
  }

  switch (knob_idx) {
    case 0:
      edit_pos = value / 22;
      set_display_mode(EDIT_SCALE);
      break;
    case 2:
      edit_preset_num = value / 16;
      edit_scale = preset_info.scale_preset.scales[edit_preset_num];
      set_display_mode(SELECT_PRESET);
      break;
    default:
      break;
  }
}

void set_current_value_on_pattern(uint8_t value, uint8_t knob_idx){
  update_knob_history(knob_idx);

  if (is_knob_multi(2, 4)) {
    // select scale edit
    enter_edit_scale_mode();
    reset_knob_history(knob_idx);
    return;
  }

  switch (knob_idx) {
    case 0:
      edit_pos = value / 16;
      set_display_mode(EDIT_PATTERN);
      break;
    case 1:
      edit_pattern[edit_pos] = value / 16;
      set_led_count(edit_pattern[edit_pos]);
      set_display_mode(COUNT);
      break;
    case 3: // fill (= preset select)
      edit_preset_num = value / 16;
      memcpy(edit_pattern, preset_info.pattern_preset.patterns[edit_preset_num], 16);
      set_display_mode(SELECT_PRESET);
      break;
    default:
      break;
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

  switch(edit_mode) {
    case NORMAL:
      press_on_normal(button_idx);
      break;
    case SELECT:
      press_on_select(button_idx);
      break;
    case SCALE:
      press_on_scale(button_idx);
      break;
    case PATTERN:
      press_on_pattern(button_idx);
      break;
    default:
      break;
  }
}

void press_on_normal(uint8_t button_idx) {
  switch(button_idx) {
    case 0:
      current_state.func = 1;
      if (is_multi_tap(button_idx, 5) && button_history.interval_tick < 65535) {
        enter_edit_select_mode();
      }
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

void press_on_select(uint8_t button_idx){
  switch(button_idx) {
    case 0:
      if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 65535) {
        leave_edit_select_mode();
      }
      break;
    default:
      break;
  }
}
void press_on_scale(uint8_t button_idx){
  switch(button_idx) {
    case 0:
      if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 65535) {
        leave_edit_select_mode();
      }
      break;
    case 1:
      reset_scale_preset(edit_preset_num);
      edit_scale = preset_info.scale_preset.scales[edit_preset_num];
      break;
    case 2:
      if (edit_scale & (1<<edit_pos)) {
        edit_scale &= ~(1<<edit_pos);
      } else {
        edit_scale |= (1<<edit_pos);
      }
      break;
    case 3:
      write_scale_preset(edit_preset_num, edit_scale);
      break;
    default:
      break;
  }
}
void press_on_pattern(uint8_t button_idx){
  switch(button_idx) {
    case 0:
      if (is_multi_tap(button_idx, 2) && button_history.interval_tick < 65535) {
        leave_edit_select_mode();
      }
      break;
    case 1:
      reset_pattern_preset(edit_preset_num);
      memcpy(edit_pattern, preset_info.pattern_preset.patterns[edit_preset_num], 16);
      break;
    case 3:
      write_pattern_preset(edit_preset_num, edit_pattern);
      break;
    default:
      break;
  }
}

void enter_edit_select_mode() {
  edit_mode = SELECT;
  set_display_mode(EDIT_SELECT);
  stop_seq();
}

void leave_edit_select_mode() {
  edit_mode = NORMAL;
  set_display_mode(SEQ);
  stop_seq();
}

void enter_edit_scale_mode(){
  edit_mode = SCALE;
  set_display_mode(EDIT_SCALE_SELECT);
  start_seq();
}

void enter_edit_pattern_mode(){
  edit_mode = PATTERN;
  set_display_mode(EDIT_PATTERN_SELECT);
  start_seq();
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
    if (edit_mode == NORMAL && (TCNT1 == 0 || TCNT1 > 50)) {
      if (!current_state.start) {
        start_trigger();
      }
      step_seq();
    }
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
    if (changed_value_flags & (1<<i)) {
      recorded_values[record_pos].values[i] = current_values.values[i];
    } else {
      recorded_values[record_pos].values[i] = 0xFF; // no record
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
  uint8_t is_change_seq = 0;
  for (int i = 0; i < sizeof(ControllerValue); ++i) {
    if (!(changed_value_flags & (1<<i)) && recorded_values[record_pos].values[i] != 0xFF) {
      current_values.values[i] = recorded_values[record_pos].values[i];
      if (i < 3) { // step_fill, step_length, step_rot
        is_change_seq = 1;
      }
    }
  }
  if (is_change_seq) {
    update_seq_pattern();
  }
}

void clear_recording() {
  memset(&recorded_values, 0xFF, sizeof(ControllerValue) * 64);
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

  record_start = 0;
  record_end = 0;
  record_length = 0;
  record_pos = 0;
  rec_mode = STOP;
  func_mode = NONE;
  button_history.mode = NONE;
  button_history.button_idx = 5;
  memset(knob_values, 0, 4*KNOB_VALUES_SIZE);
  memset(button_state, 0, 4);
  changed_value_flags = 0;
  memset(&recorded_values, 0, sizeof(ControllerValue) * 64);

  edit_preset_num = 0;
  edit_pos = 0;
  edit_scale = 1;
  memset(&edit_pattern, 0, 16);
}
