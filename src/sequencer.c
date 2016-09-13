#include <avr/pgmspace.h>

#include "sequencer.h"
#include "input.h"
#include "timer.h"
#include "euclid.h"
#include "pattern.h"
#include "scale.h"
#include "dac.h"
#include "eeprom.h"
#include "variables.h"

volatile uint8_t divide_idx = 1;
volatile uint8_t pitch_duration_quantized = 0;

static uint8_t _step = 0;
static uint8_t seq_start_shift = 0;

void step_seq() {
  switch(edit_mode) {
    case NORMAL:
      step_seq_on_normal();
      break;
    case SCALE:
      step_seq_on_edit_scale();
      break;
    case PATTERN:
      step_seq_on_edit_pattern();
      break;
    default:
      break;
  }
}

void step_seq_on_normal(){
  if (divide_idx < divide_count) {
    ++divide_idx;
    return;
  }
  divide_idx = 1;

  cli();
  if (_step < (current_values.v.step_length-1)) {
    ++_step;
    current_step = (_step + seq_start_shift) % 16;
  } else {
    _step = 0;
    current_step = seq_start_shift;
    update_seq_pattern();
  }
  sei();

  start_gate_timer();

  update_knob_values();

  cli();
  if (rec_mode == PLAY || rec_mode == REC) {
    play_recorded_knob_values();
    if (rec_mode == REC) {
      record_current_knob_values();
    } else {
      next_play_pos();
    }
  }
  sei();

  if (active_seq[current_step]) {
    cli();
    update_pitch();
    sei();
  }
  cli();
  update_slide();
  sei();
  cli();
  update_wave_shape();

  changed_value_flags = 0;
  sei();
}

static volatile char current_test_note = -1;
void step_seq_on_edit_scale(){
  update_knob_values();
  cli();
  char found = 0;
  for (char i=0; i<12 && !found; ++i) {
    if (edit_scale & (1<<i)) {
      found = 1;
    }
  }
  if (!found) {
    current_test_note = -1;
    sei();
    return;
  }
  char count = 0;
  current_test_note = (current_test_note+1) % 12;
  while (!(edit_scale & (1<<current_test_note)) && count < 12) {
    current_test_note = (current_test_note+1) % 12;
    ++count;
  }
  current_pitch1 = current_test_note + 64;
  update_oct_note();
  start_gate_timer();
  sei();
}

static volatile uint8_t current_test_pos = 0;
void step_seq_on_edit_pattern(){
  update_knob_values();
  cli();
  uint8_t value = edit_pattern[current_test_pos];
  current_pitch1 = value + 64;
  update_oct_note();
  start_gate_timer();
  current_test_pos = (current_test_pos+1) % 16;
  sei();
}

// trig in when stop sequence
void start_trigger() {
  TCNT1 = 0;
  TCCR1B |= (1<<CS12); // divide 256
  OCR1A = 0xFFFF;
}

void start_seq() {
  current_state.start = 1;
  srand(button_history.last_tick);
  TCNT1 = 0;
  TCCR1B |= (1<<CS12); // divide 256
  reset_phase();
}

void stop_seq() {
  current_state.start = 0;
  TCCR1B &= ~(1<<CS12);
  TCNT1 = 0;
}

void reset_seq() {
  _step = 16; // next -> 0
  current_step = seq_start_shift > 0 ? seq_start_shift - 1 : 16;
  reset_phase();
}

void start_gate_timer() {
  //reset timer counter
  TCNT0 = 0; 
  //start timer
  TCCR0B = (1<<CS02) | (1<<CS00); // divide 1024
  
  if (active_seq[current_step]) {
    //gate on
    PORTB &= ~_BV(2);
  }

  active_step_gate = 1;
}

void update_step_time() {
  if (current_values.v.swing > 0) {
    uint16_t offset_interval = ((long)step_interval * current_values.v.swing) / 255;
    if (current_step % 2 == 0) {
      OCR1A = step_interval + offset_interval;
    } else {
      OCR1A = step_interval - offset_interval;
    }
  } else {
    OCR1A = step_interval;
  }
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
  uint16_t euclid_seq = pgm_read_word(&(euclid_seq_table[current_values.v.step_length-1][current_values.v.step_fill]));
  uint8_t steplen = current_values.v.step_length;
  uint8_t steprot = current_values.v.step_rot;
  if (steprot >= 16) {
    // rot start
    steprot = steprot - 16;
    seq_start_shift = steprot;
    for (uint8_t i = 0; i < 16; ++i) {
      if (i < steplen && steprot > 0) {
        active_seq[(i+seq_start_shift)%16] = !!(euclid_seq & (1 << ((i+steprot)%steplen)));
      } else {
        active_seq[(i+seq_start_shift)%16] = !!(euclid_seq & (1 << i));
      }
    }
  } else {
    // rot trigger
    seq_start_shift = 0;
    for (uint8_t i = 0; i < 16; ++i) {
      if (i < steplen && steprot > 0) {
        active_seq[i] = !!(euclid_seq & (1 << ((i + steprot)%steplen)));
      } else {
        active_seq[i] = !!(euclid_seq & (1 << i));
      }
    }
  }
  randomize_seq();
}

void randomize_seq() {
  if (current_values.v.step_rand > 0) {
    for (uint8_t i = 0; i < current_values.v.step_length; ++i) {
      if ((uint8_t)(rand() >> 8) < current_values.v.step_rand) {
        uint8_t step = (i + seq_start_shift) % 16;
        active_seq[step] = !active_seq[step];
      }
    }
  }
}

uint8_t quantize_pitch(uint8_t pitch) {
  uint8_t result;
  uint8_t base_value = pitch / 12 * 12;
  uint8_t upper_value = pitch - base_value;
  upper_value = scale_table[current_values.v.scale_select][upper_value];
  int tmp = base_value + upper_value + (current_values.v.scale_transpose - 36);
  if (tmp > 119) {
    result = 119;
  } else if (tmp < 0) {
    result = 0;
  } else {
    result= tmp;
  }
  return result;
}

void update_pitch() {
  prev_pitch1     = current_pitch1;
  prev_pitch2     = current_pitch2;
  prev_oct1       = current_oct1;
  prev_note_num1  = current_note_num1;
  prev_oct2       = current_oct2;
  prev_note_num2  = current_note_num2;
  int pattern_value = ((preset_info.pattern_preset.patterns[current_values.v.scale_pattern][current_step] - 8) * current_values.v.scale_range);
  int rand_value = (((int)current_values.v.scale_pattern_random * ((int)(rand() >> 12) - 8)));
  int tmp_value = ((pattern_value + rand_value) / 5) + current_values.v.scale_shift;
  if (tmp_value < 0) {
    tmp_value = 0;
  }

  current_pitch1 = quantize_pitch(tmp_value);
  if (pitch_duration_quantized) {
    current_pitch2 = quantize_pitch(current_pitch1 + pitch_duration);
  } else {
    uint8_t pitch_tmp = current_pitch1 + pitch_duration;
    if (pitch_tmp > 119) {
      pitch_tmp = 119;
    }
    current_pitch2 = pitch_tmp;
  }
  if (prev_pitch1 != current_pitch1 || prev_pitch2 != current_pitch2) {
    update_oct_note();
  }
}

void update_oct_note() {
  prev_oct1       = current_oct1;
  prev_note_num1  = current_note_num1;
  prev_oct2       = current_oct2;
  prev_note_num2  = current_note_num2;
  current_oct1       = current_pitch1 / 12;
  current_note_num1  = current_pitch1 % 12;
  current_oct2       = current_pitch2 / 12;
  current_note_num2  = current_pitch2 % 12;
  current_pitch1_dec = current_pitch1 << 8;
  current_pitch2_dec = current_pitch2 << 8;
  reset_count_in_cycle();
}

void update_slide() {
  if (prev_pitch1 != current_pitch1 && active_seq[current_step] && prev_pitch1 < 120 && current_values.v.slide > 0) {
    slide_speed = (16 - current_values.v.slide);
    slide_pitch1 = prev_pitch1 << 8;
    slide_pitch2 = prev_pitch2 << 8;
    slide_buf_value1 = 0;
    slide_buf_value2 = 0;
  } else {
    slide_speed = 0;
  }
}

void update_wave_shape() {
  // 下位3bitがwave2のidx、上位がwave1のidx
  selected_wavetable_type1 = current_values.v.wave_select >> 3;
  uint8_t tmp_wave_type2 = (current_values.v.wave_select & 0x07);
  if (tmp_wave_type2 == 7) {
    // 意図的にwavetableのindex越えさせてノイズを発生
    selected_wavetable_type2 = 4;
  } else {
    selected_wavetable_type2 = tmp_wave_type2 & 0x03;
    selected_wavetable_type2_sign = !(tmp_wave_type2 & 0x04);
  }

  if (changed_value_flags & (_BV(CHG_VAL_FLAG_WAVE_PHASE) | _BV(CHG_VAL_FLAG_WAVE_PITCH_DURATION))) {
    reset_phase();
  }

  if (current_values.v.wave_phase < 16) {
    wave_phase_shift = current_values.v.wave_phase;
    wave_phase_shift_cycle = 0;
  } else {
    wave_phase_shift = 0;
    wave_phase_shift_cycle = (current_values.v.wave_phase - 15);
  }

  wave1_volume = 8 - current_values.v.wave_balance;
  wave2_volume = current_values.v.wave_balance;

  if (current_values.v.wave_pitch_duration < 16) {
    pitch_duration_quantized = 0;
    pitch_duration = current_values.v.wave_pitch_duration;
  } else {
    pitch_duration_quantized = 1;
    pitch_duration = current_values.v.wave_pitch_duration - 15;
  }
}
