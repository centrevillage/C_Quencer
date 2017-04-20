#include "input.h"
#include "variables.h"
#include "sequencer.h"
#include "timer.h"
#include "led.h"
#include "eeprom.h"
#include "adc.h"

volatile ControllerValue current_values;
volatile NoRecValue no_rec_values;

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

volatile uint8_t knob_values[KNOB_COUNT][KNOB_VALUES_SIZE];
volatile uint8_t current_knob_idx = 0;
volatile uint8_t current_knob_value_idx = 0;

static volatile uint8_t record_start = 0;
static volatile uint8_t record_end = 0;
static volatile uint8_t record_length = 0;
static volatile uint8_t record_pos = 0;
static volatile uint8_t play_pos = 0;

volatile uint16_t prev_values[] = {0, 0, 0, 0};

volatile uint32_t tap_tempo_hp_tick = 0;
volatile uint8_t in_tap_tempo = 0;
volatile uint32_t tap_tempo_interval;

void leave_on_rec_mode();

inline void update_knob_value_inline(uint8_t i) {
  uint16_t prev_value;
  uint16_t new_value_sum;
  uint16_t new_value;

  prev_value = prev_values[i];

  //new_value_sum = 0;
  //for (uint8_t j = 0; j < KNOB_VALUES_SIZE; ++j) {
  //  new_value_sum += knob_values[i][j];
  //}
  new_value_sum =  knob_values[i][0];
  new_value_sum += knob_values[i][1];
  new_value_sum += knob_values[i][2];
  new_value_sum += knob_values[i][3];
  new_value_sum += knob_values[i][4];
  new_value_sum += knob_values[i][5];
  new_value_sum += knob_values[i][6];
  new_value_sum += knob_values[i][7];

  int diff = new_value_sum - prev_value * KNOB_VALUES_SIZE;
  if (diff >= KNOB_VALUES_SIZE) {
    new_value = prev_value + diff / KNOB_VALUES_SIZE;
    set_current_value((uint8_t)new_value, i);
    prev_values[i] = new_value;
  } else if (diff <= -KNOB_VALUES_SIZE) {
    new_value = prev_value - ((-diff) / KNOB_VALUES_SIZE);
    set_current_value((uint8_t)new_value, i);
    prev_values[i] = new_value;
  }
}

inline void check_timeout() {
  if (ticks() - button_history.last_tick > MULTI_TAP_TIMEOUT_INTERVAL) {
    reset_button_history(button_history.button_idx);
  }

  if (in_tap_tempo) {
    if (hp_ticks() - tap_tempo_hp_tick > TAP_TEMPO_TIMEOUT) {
      in_tap_tempo = 0;
    }
  }
}

inline void clear_recording() {
  memset(&recorded_values, 0xFF, sizeof(ControllerValue) * RECORDED_VALUES_SIZE);
}

void update_knob_values() {
  cli(); check_timeout(); sei();
  cli(); update_knob_value_inline(0); sei();
  cli(); update_knob_value_inline(1); sei();
  cli(); update_knob_value_inline(2); sei();
  cli(); update_knob_value_inline(3); sei();
}

void reset_knob_history(uint8_t knob_idx) {
  uint16_t t = ticks();
  knob_history.knob_idx = knob_idx;
  knob_history.count = 1;
  knob_history.last_tick = t;
  knob_history.interval_tick = 0;
}

void update_knob_history(uint8_t knob_idx) {
  if (knob_history.knob_idx == knob_idx) {
    uint16_t t = ticks();
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
  uint8_t tmp_value;
  switch (knob_idx) {
    case 0: // fill / len / slide / wave select / clock instability
      switch (func_mode) {
        case NONE:
          current_values.v.step_fill = value >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_FILL;
          break;
        case FUNC:
          current_values.v.step_length = (value >> 4) + 1;
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_LENGTH;
          break;
        case HID:
          current_values.v.slide = value >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SLIDE;
          set_led_count((value >> 4) + 1);
          break;
        case WAVE_SHAPE:
          current_values.v.wave_select = value >> 3;
          changed_value_flags |= 1<<CHG_VAL_FLAG_WAVE_SELECT;
          set_display_mode(WAVE_SHAPE_SELECT);
          break;
        case STABILITY:
          no_rec_values.v.int_clock_instability = value >> 2;
          set_disp_left_8_dot_right_8_val(no_rec_values.v.int_clock_instability);
          break;
      }
      break;
    case 1: // rot / rand / swing / wave phase / clock sync
      switch (func_mode) {
        case NONE:
          current_values.v.step_rot = value >> 3;
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_ROT;
          break;
        case FUNC:
          current_values.v.step_rand = value >> 3;
          changed_value_flags |= 1<<CHG_VAL_FLAG_STEP_RAND;
          set_led_count((value >> 4) + 1);
          break;
        case HID:
          current_values.v.swing = value >> 1;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SWING;
          set_led_count((value >> 4) + 1);
          break;
        case WAVE_SHAPE:
          current_values.v.wave_phase = value >> 3;
          changed_value_flags |= 1<<CHG_VAL_FLAG_WAVE_PHASE;
          set_led_count((value >> 3) + 1);
          break;
        case STABILITY:
          no_rec_values.v.int_clock_sync_to_ext = value >> 4;
          set_led_count((value >> 4) + 1);
          break;
      }
      break;
    case 2: // scale select / transpose / scale pattern random / wave balance / pitch vibrato
      switch (func_mode) {
        case NONE:
          current_values.v.scale_select = value >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_SELECT;
          set_led_count((value >> 4) + 1);
          break;
        case FUNC:
          current_values.v.scale_transpose = (uint16_t)value * 76 / 256;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_TRANSPOSE;
          set_display_mode(TRANSPOSE);
          break;
        case HID:
          current_values.v.scale_shift = (value >> 2) + 16;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_SHIFT;
          set_led_count((value >> 4) + 1);
          break;
        case WAVE_SHAPE:
          tmp_value = value >> 4;
          if (tmp_value < 4) {
            tmp_value = 4;
          } else if (tmp_value > 12) {
            tmp_value = 12;
          }
          tmp_value -= 4;
          current_values.v.wave_balance = tmp_value;
          changed_value_flags |= 1<<CHG_VAL_FLAG_WAVE_BALANCE;
          set_display_mode(WAVE_SHAPE_BALANCE);
          break;
        case STABILITY:
          no_rec_values.v.pitch_vibrato = value >> 2;
          set_disp_left_8_dot_right_8_val(value);
          break;
      }
      break;
    case 3: // scale pattern / scale range / scale pattern random / wave pitch duration / pitch overshoot
      switch (func_mode) {
        case NONE:
          current_values.v.scale_pattern = value >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_PATTERN;
          set_led_count((value >> 4) + 1);
          break;
        case FUNC:
          current_values.v.scale_range = (value >> 4) + 1;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_RANGE;
          set_led_count((value >> 4) + 1);
          break;
        case HID:
          current_values.v.scale_pattern_random = value >> 4;
          changed_value_flags |= 1<<CHG_VAL_FLAG_SCALE_PAT_RAND;
          set_led_count((value >> 4) + 1);
          break;
        case WAVE_SHAPE:
          current_values.v.wave_pitch_duration = value >> 3;
          changed_value_flags |= 1<<CHG_VAL_FLAG_WAVE_PITCH_DURATION;
          set_led_count((value >> 3) + 1);
          break;
        case STABILITY:
          no_rec_values.v.pitch_overshoot = value >> 2;
          set_disp_left_8_dot_right_8_val(no_rec_values.v.pitch_overshoot);
          break;
      }
      break;
    default:
      break;
  }
  if (changed_value_flags & (_BV(CHG_VAL_FLAG_STEP_FILL) | _BV(CHG_VAL_FLAG_STEP_LENGTH) | _BV(CHG_VAL_FLAG_STEP_ROT))) {
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
      set_led_count(edit_pattern[edit_pos] + 1);
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
  if (current_state.wave_shape) {
    if (current_state.func) {
      return STABILITY;
    } else {
      return WAVE_SHAPE;
    }
  } else if (current_state.hid) {
    return HID;
  } else if (current_state.func) {
    return FUNC;
  }
  return NONE;
}

void reset_button_history(uint8_t button_idx) {
    uint16_t t = ticks();
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
    uint16_t t = ticks();
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
    uint16_t t = ticks();
    button_history.last_leave = t;
  } else {
    button_history.last_leave = 0;
  }
};

void press(uint8_t button_idx) {
  ButtonHistory prev_button_history = button_history;

  update_button_history(button_idx);

  if (is_multi_tap(button_idx, 2)) {
    if (button_history.interval_tick < FLUTTERING_INTERVAL) {
      // avoid fluttering
      button_history = prev_button_history;
      return;
    } else if (button_history.interval_tick > MULTI_TAP_TIMEOUT_INTERVAL) {
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
  uint32_t hp_tick;
  switch(button_idx) {
    case 0:
      current_state.func = 1;
      if (is_multi_tap(button_idx, 5)) {
        enter_edit_select_mode();
        reset_button_history(button_idx);
      }
      break;
    case 1:
      switch (func_mode) {
        case NONE: // START/STOP
          if (current_state.start) {
            stop_seq();
          } else {
            start_seq();
          }
          break;
        case FUNC: // RESET
          reset_seq();
          break;
        case HID:
          current_state.wave_shape = 1;
          break;
        default:
          break;
      }
      break;
    case 2:
      if (func_mode == FUNC) {
        if (is_multi_tap(button_idx, 2)) {
          set_divide(button_history.count);
        } else {
          set_divide(1);
          reset_button_history(button_idx);
        }
      } else {
        current_state.hid = 1;
        hp_tick = hp_ticks();
        if (in_tap_tempo == 2) {
          tap_tempo_interval = hp_tick - tap_tempo_hp_tick;
          if (tap_tempo_interval > TAP_TEMPO_TIMEOUT) {
            in_tap_tempo = 0;
          } else {
            tap_tempo_hp_tick = hp_tick;
          }
        } else {
          in_tap_tempo = 1;
          tap_tempo_hp_tick = hp_tick;
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
          play_pos = 0;
          end_recording();
        }
      }
      break;
    default:
      break;
  }
  if (button_idx != 2) {
    in_tap_tempo = 0;
  }
  func_mode = get_func_mode();
}

void press_on_select(uint8_t button_idx){
  switch(button_idx) {
    case 0:
      if (is_multi_tap(button_idx, 2)) {
        leave_edit_select_mode();
        reset_button_history(button_idx);
      }
      break;
    default:
      break;
  }
}
void press_on_scale(uint8_t button_idx){
  switch(button_idx) {
    case 0:
      if (is_multi_tap(button_idx, 2)) {
        leave_edit_select_mode();
        reset_button_history(button_idx);
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
      if (is_multi_tap(button_idx, 2)) {
        leave_edit_select_mode();
        reset_button_history(button_idx);
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
  if (current_state.start) {
    stop_seq();
  }
}

void leave_edit_select_mode() {
  edit_mode = NORMAL;
  set_display_mode(SEQ);
  if (current_state.start) {
    stop_seq();
  }
}

void enter_edit_scale_mode(){
  edit_mode = SCALE;
  set_display_mode(EDIT_SCALE_SELECT);
  if (!current_state.start) {
    start_seq();
  }
}

void enter_edit_pattern_mode(){
  edit_mode = PATTERN;
  set_display_mode(EDIT_PATTERN_SELECT);
  if (!current_state.start) {
    start_seq();
  }
}

void leave(uint8_t button_idx) {
  update_button_history_on_leave(button_idx);
  switch(button_idx) {
    case 0:
      current_state.func = 0;
      break;
    case 1:
      current_state.wave_shape = 0;
      break;
    case 2:
      current_state.hid = 0;
      if (in_tap_tempo) {
        if (hp_ticks() - tap_tempo_hp_tick > TAP_FUNC_DURAITON) { // TAPƒ{ƒ^ƒ“0.8•bˆÈã’·‰Ÿ‚µ
          in_tap_tempo = 0;
        } else {
          if (in_tap_tempo == 2) {
            set_step_interval(tap_tempo_interval/16);
          }
          in_tap_tempo = 2;
        }
      }
      break;
    case 3:
      if (rec_mode == REC && record_length > 7) {
        leave_on_rec_mode();
      }
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
  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t pin = i + 5;
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


void leave_on_rec_mode() {
  // quantized rec
  uint8_t quantized_len = ((record_length + current_values.v.step_length/2) / current_values.v.step_length) * current_values.v.step_length;
  while (quantized_len > RECORDED_VALUES_SIZE) {
    quantized_len -= current_values.v.step_length;
  }
  char rec_len_diff = record_length - quantized_len;
  if (record_length > quantized_len) {
    record_length = quantized_len;
    char record_pos_tmp =  record_pos - (record_length - quantized_len);
    if (record_pos_tmp < 0) {
      record_pos = RECORDED_VALUES_SIZE + record_pos_tmp;
    } else {
      record_pos = record_pos_tmp;
    }
    play_pos = rec_len_diff;
  } else if (record_length < quantized_len) {
    fill_remains_records(quantized_len);
    play_pos = record_length + rec_len_diff;
  } else {
    play_pos = 0;
  }
  rec_mode = PLAY;
  end_recording();
}

void next_play_pos() {
  ++play_pos;
  if (play_pos >= record_length) {
    play_pos = 0;
  }
  record_pos = (play_pos + record_start) % RECORDED_VALUES_SIZE;
}

void record_current_knob_values() {
  for (uint8_t i = 0; i < sizeof(ControllerValue); ++i) {
    if (changed_value_flags & (1<<i)) {
      recorded_values[record_pos].values[i] = current_values.values[i];
    } else {
      recorded_values[record_pos].values[i] = 0xFF; // no record
    }
  }

  if (record_length < RECORDED_VALUES_SIZE) {
    record_length++;
  }

  record_pos = (record_pos + 1) % RECORDED_VALUES_SIZE;
}

void fill_remains_records(uint8_t quantized_len) {
  while (record_length < quantized_len) {
    for (uint8_t i = 0; i < sizeof(ControllerValue); ++i) {
      recorded_values[record_pos].values[i] = 0xFF; // no record
    }
    if (record_length < RECORDED_VALUES_SIZE) {
      record_length++;
    }
    record_pos = (record_pos + 1) % RECORDED_VALUES_SIZE;
  }
}

void start_recording() {
  record_start = record_pos;
  record_length = 0;
  current_state.rec = 1;
}

void end_recording() {
  record_end = record_pos;
  int record_start_tmp = ((int)record_end) - ((int)record_length);
  record_start = (uint8_t)(record_start_tmp < 0 ? (record_start_tmp + RECORDED_VALUES_SIZE) : record_start_tmp);
  current_state.rec = 0;
}

void play_recorded_knob_values() {
  if (record_length == 0) {
    return;
  }
  uint8_t changed_seq_flags = changed_value_flags & (_BV(CHG_VAL_FLAG_STEP_FILL) | _BV(CHG_VAL_FLAG_STEP_LENGTH) | _BV(CHG_VAL_FLAG_STEP_ROT));
  for (uint8_t i = 0; i < sizeof(ControllerValue); ++i) {
    if (!(changed_value_flags & (1<<i)) && recorded_values[record_pos].values[i] != 0xFF) {
      current_values.values[i] = recorded_values[record_pos].values[i];
      changed_value_flags |= (1<<i);
    }
  }
  uint8_t changed_seq_flags_after_play = changed_value_flags & (_BV(CHG_VAL_FLAG_STEP_FILL) | _BV(CHG_VAL_FLAG_STEP_LENGTH) | _BV(CHG_VAL_FLAG_STEP_ROT));
  if (changed_seq_flags != changed_seq_flags_after_play) {
    update_seq_pattern();
  }
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
  current_values.v.wave_select = 0;
  current_values.v.wave_balance = 4;
  current_values.v.wave_phase = 0;
  current_values.v.wave_pitch_duration = 0;

  record_start = 0;
  record_end = 0;
  record_length = 0;
  record_pos = 0;
  play_pos = 0;
  rec_mode = STOP;
  func_mode = NONE;
  button_history.mode = NONE;
  button_history.button_idx = 5;
  memset(knob_values, 0, KNOB_COUNT*KNOB_VALUES_SIZE);
  memset(button_state, 0, 4);
  changed_value_flags = 0;

  clear_recording();

  edit_preset_num = 0;
  edit_pos = 0;
  edit_scale = 1;
  memset(&edit_pattern, 0, 16);
}

void read_knob_values_exec(uint8_t pin, uint8_t data) {
  knob_values[pin][current_knob_value_idx] = 0xFF - data;
  ++current_knob_idx;
  if (current_knob_idx >= KNOB_COUNT) {
    current_knob_idx = 0;
    current_knob_value_idx = (current_knob_value_idx + 1) % KNOB_VALUES_SIZE;
  }
}

void read_knob_values_async() {
  adc_async_read(current_knob_idx, read_knob_values_exec);
}
