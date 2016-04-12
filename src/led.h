#include <avr/io.h>

#ifndef CQ_LED_H_
#define CQ_LED_H_

#define LED_MASK 0b00011111
#define MAX_DISPLAY_TICK_FOR_VALUE 16000

enum DisplayMode {
  SEQ,
  COUNT
};

void output_led_on_seq();
void output_led_on_value();
void output_led();
void set_display_mode(enum DisplayMode mode);
void set_led_count(uint8_t led_count, uint8_t tick);


#endif /* CQ_LED_H_ */
