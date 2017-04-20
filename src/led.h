#include <avr/io.h>

#ifndef CQ_LED_H_
#define CQ_LED_H_

#define LED_MASK 0b00011111
#define MAX_DISPLAY_TICK_FOR_VALUE 512
#define MAX_DISPLAY_TICK_FOR_REC_CLEAR 512

enum DisplayMode {
  SEQ,
  COUNT,
  TRANSPOSE,
  REC_CLEAR,
  REC_REDO,
  EDIT_SELECT,
  EDIT_SCALE,
  EDIT_PATTERN,
  EDIT_SCALE_SELECT,
  EDIT_PATTERN_SELECT,
  SELECT_PRESET,
  WAVE_SHAPE_SELECT,
  WAVE_SHAPE_BALANCE,
  LEFT_8_DOT_RIGHT_8_VAL
};

void output_led_on_seq();
void output_led_on_value();
void output_led_on_transpose_key();
void output_led_on_transpose();
void output_led_on_rec_clear();
void output_led_on_edit_select();
void output_led_on_edit_scale();
void output_led_on_edit_pattern();
void output_led_on_edit_scale_select();
void output_led_on_edit_pattern_select();
void output_led_on_select_preset();
void output_led_on_wave_select();
void output_led_on_wave_balance();
void output_led_on_left_8_right_8();
void output_led();
void set_display_mode(enum DisplayMode mode);
void set_led_count(uint8_t led_count);
void set_disp_left_8_dot_right_8_val(uint8_t led_count);

#endif /* CQ_LED_H_ */
