#include <avr/pgmspace.h>

#include "sequencer.h"
#include "input.h"
#include "timer.h"
#include "euclid.h"
#include "pattern.h"
#include "scale.h"
#include "dac.h"
#include "eeprom.h"

volatile unsigned char active_seq[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t current_step = 0;
// 1 tick(timer1) = 16 usec, 120 BPM = 2sec / 16 usec / 16 = 7812.5 tick;
volatile uint16_t step_interval = 8000L;
volatile uint8_t  divide_count = 1;
volatile uint8_t  divide_idx = 1;
volatile uint8_t active_step_gate = 0;
volatile uint8_t current_pitch = 0;

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
  cli();
  divide_idx = 1;

  if (_step < (current_values.v.step_length-1)) {
    ++_step;
    current_step = (_step + seq_start_shift) % 16;
  } else {
    _step = 0;
    current_step = seq_start_shift;
    update_seq_pattern();
  }
  update_knob_values();
  if (rec_mode == PLAY || rec_mode == REC) {
    play_recorded_knob_values();
    if (rec_mode == REC) {
      record_current_knob_values();
    } else {
      next_play_pos();
    }
  }
  if (active_seq[current_step]) {
    reset_phase_shift();
    update_pitch();
  }
  start_gate_timer();

  changed_value_flags = 0;
  sei();
}

static volatile char current_test_note = -1;
void step_seq_on_edit_scale(){
  cli();
  update_knob_values();
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
  current_pitch = current_test_note + 64;
  start_gate_timer();
  sei();
}

static volatile uint8_t current_test_pos = 0;
void step_seq_on_edit_pattern(){
  cli();
  update_knob_values();
  uint8_t value = edit_pattern[current_test_pos];
  current_pitch = value + 64;
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
}

void stop_seq() {
  current_state.start = 0;
  TCCR1B &= ~(1<<CS12);
  TCNT1 = 0;
}

void reset_seq() {
  _step = 16; // next -> 0
  current_step = seq_start_shift > 0 ? seq_start_shift - 1 : 16;
}

void start_gate_timer() {
  //reset timer counter
  TCNT0 = 0; 
  //start timer
  TCCR0B |= (1<<CS02) | (1<<CS00); // divide 1024
  
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
    for (int i = 0; i < 16; ++i) {
      if (i < steplen && steprot > 0) {
        active_seq[(i+seq_start_shift)%16] = !!(euclid_seq & (1 << ((i+steprot)%steplen)));
      } else {
        active_seq[(i+seq_start_shift)%16] = !!(euclid_seq & (1 << i));
      }
    }
  } else {
    // rot trigger
    seq_start_shift = 0;
    for (int i = 0; i < 16; ++i) {
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

uint8_t prev_pitch = 255;
void update_pitch() {
  prev_pitch = current_pitch;
  int pattern_value = ((preset_info.pattern_preset.patterns[current_values.v.scale_pattern][current_step] - 8) * current_values.v.scale_range);
  int rand_value = (((int)current_values.v.scale_pattern_random * ((int)((rand() & 0xF000) >> 12) - 8)));
  int tmp_value = ((pattern_value + rand_value) / 5) + current_values.v.scale_shift;
  if (tmp_value < 0) {
    tmp_value = 0;
  }

  // quantize
  int base_value = tmp_value / 12 * 12;
  int upper_value = tmp_value - base_value;
  upper_value = scale_table[current_values.v.scale_select][upper_value];
  int current_pitch_tmp = base_value + upper_value + (current_values.v.scale_transpose - 36);
  if (current_pitch_tmp > 119) {
    current_pitch = 119;
  } else if (current_pitch_tmp < 0) {
    current_pitch = 0;
  } else {
    current_pitch = current_pitch_tmp;
  }
}
