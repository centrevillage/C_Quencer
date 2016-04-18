#include <avr/io.h>

#ifndef CQ_LED_H_
#define CQ_LED_H_

#define LED_MASK 0b00011111
#define MAX_DISPLAY_TICK_FOR_VALUE 65535

enum DisplayMode {
  SEQ,
  COUNT,
  SCALE,
  TRANSPOSE
};

void output_led_on_seq();
void output_led_on_value();
void output_led_on_scale();
void output_led_on_transpose_key();
void output_led_on_transpose();
void output_led();
void set_display_mode(enum DisplayMode mode);
void set_led_count(uint8_t led_count);

#endif /* CQ_LED_H_ */
