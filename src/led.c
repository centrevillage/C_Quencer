#include "led.h"
#include "sequencer.h"
#include "timer.h"
#include "input.h"
#include <avr/pgmspace.h>

static volatile enum DisplayMode display_mode = SEQ;
static volatile uint8_t led_count = 0;
static volatile unsigned long last_led_disp_tick = 0;
static uint8_t rec_led_count = 0;
static uint8_t led_blink_count = 0;

static int g_led_i = 0;

static const uint8_t led_idx_to_port_and_ddr[16][2] PROGMEM = {
  {0b00000010, 0b00000011},
  {0b00000001, 0b00000011},
  {0b00000100, 0b00000101},
  {0b00000001, 0b00000101},

  {0b00000100, 0b00000110},
  {0b00000010, 0b00000110},
  {0b00001000, 0b00001010},
  {0b00000010, 0b00001010},

  {0b00001000, 0b00001100},
  {0b00000100, 0b00001100},
  {0b00010000, 0b00010100},
  {0b00000100, 0b00010100},

  {0b00010000, 0b00011000},
  {0b00001000, 0b00011000},
  {0b00010000, 0b00010001},
  {0b00000001, 0b00010001}
};

void output_led_on_seq() {
  if (active_seq[g_led_i]) {
    if (!(current_step == g_led_i && active_step_gate)) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  } else {
    if (current_step == g_led_i && active_step_gate) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  }
}

void output_led_on_value() {
  if (g_led_i < led_count) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}

void output_led_on_transpose_key() {
  uint8_t v = current_values.v.scale_transpose % 12;
  if (v % 2 == 0) {
    if (g_led_i == 9 + (v/2)) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  } else {
    if (g_led_i == 9 + (v/2) || g_led_i == 9 + (v/2) + 1) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  }
}

void output_led_on_transpose() {
  if (g_led_i < 8) {
    uint8_t v = 7 - (current_values.v.scale_transpose / 12);
    if (g_led_i == v) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  } else {
    output_led_on_transpose_key();
  }
}

void output_led_on_rec_clear() {
  if (g_led_i == rec_led_count) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}

void output_led_on_edit_select(){
  if (g_led_i == 5 || g_led_i == 11) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}

void output_led_on_edit_scale(){
  if (g_led_i == edit_pos) {
    ++led_blink_count;
    if (led_blink_count >= 128) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  } else if (edit_scale & (1<<g_led_i)) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}

void output_led_on_edit_pattern(){
  if (g_led_i == edit_pos) {
    ++led_blink_count;
    if (led_blink_count >= 64) {
      led_blink_count = 0;
    }
    if (led_blink_count >= 32) {
      PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
      DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
    }
  }
}

void output_led_on_edit_scale_select(){
  if (led_blink_count++ < 64) {
    return;
  }
  led_blink_count = 0;

  if (g_led_i == 11) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}
void output_led_on_edit_pattern_select(){
  if (led_blink_count++ < 64) {
    return;
  }
  led_blink_count = 0;

  if (g_led_i == 5) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}

void output_led_on_select_preset() {
  if (g_led_i == edit_preset_num) {
    PORTD |= pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][0]));
    DDRD   = pgm_read_byte(&(led_idx_to_port_and_ddr[g_led_i][1]));
  }
}

// led_task
// charlieplexing
// use PD0 - PD5
void output_led() {
  if (rec_mode == REC) {
    if (led_blink_count++ < 64) {
      return;
    }
    led_blink_count = 0;
  }
  long duration;

  DDRD = 0;
  PORTD &= (uint8_t)~LED_MASK;
  switch (display_mode) {
    case SEQ:
      output_led_on_seq();
      break;
    case COUNT:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_VALUE || duration < 0 /* count wrap? */) {
        if (edit_mode == SCALE) {
          display_mode = EDIT_SCALE;
          output_led_on_edit_scale();
        } else if (edit_mode == PATTERN) {
          display_mode = EDIT_PATTERN;
          output_led_on_edit_pattern();
        } else {
          display_mode = SEQ;
          output_led_on_seq();
        }
      } else {
        output_led_on_value();
      }
      break;
    case TRANSPOSE:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_VALUE || duration < 0 /* count wrap? */) {
        display_mode = SEQ;
        output_led_on_seq();
      } else {
        output_led_on_transpose();
      }
      break;
    case REC_CLEAR:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_REC_CLEAR || duration < 0 /* count wrap? */) {
        display_mode = SEQ;
        output_led_on_seq();
      } else {
        rec_led_count = 15 - (duration/4096);
        output_led_on_rec_clear();
      }
      break;
    case REC_REDO:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_REC_CLEAR || duration < 0 /* count wrap? */) {
        display_mode = SEQ;
        output_led_on_seq();
      } else {
        rec_led_count = (duration/4096);
        output_led_on_rec_clear();
      }
      break;
    case EDIT_SELECT:
      output_led_on_edit_select();
      break;
    case EDIT_SCALE:
      output_led_on_edit_scale();
      break;
    case EDIT_PATTERN:
      output_led_on_edit_pattern();
      break;
    case EDIT_PATTERN_SELECT:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_VALUE || duration < 0 /* count wrap? */) {
        display_mode = EDIT_PATTERN;
        output_led_on_edit_pattern();
      } else {
        output_led_on_edit_pattern_select();
      }
      break;
    case EDIT_SCALE_SELECT:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_VALUE || duration < 0 /* count wrap? */) {
        display_mode = EDIT_SCALE;
        output_led_on_edit_scale();
      } else {
        output_led_on_edit_scale_select();
      }
      break;
    case SELECT_PRESET:
      duration = ticks() - last_led_disp_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_VALUE || duration < 0 /* count wrap? */) {
        if (edit_mode == SCALE) {
          display_mode = EDIT_SCALE;
          output_led_on_edit_scale();
        } else if (edit_mode == PATTERN) {
          display_mode = EDIT_PATTERN;
          output_led_on_edit_pattern();
        }
      } else {
        output_led_on_select_preset();
      }
      break;
    default:
      break;
  }

  if (++g_led_i >= 16) {
    g_led_i = 0;
  }
}

void set_display_mode(enum DisplayMode mode) {
  display_mode = mode;
  if (mode != SEQ) {
    last_led_disp_tick = ticks();
  }
}

void set_led_count(uint8_t count /* value: 1..16 */) {
  display_mode = COUNT;
  led_count = count;
  last_led_disp_tick = ticks();
}
