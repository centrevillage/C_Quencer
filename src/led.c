#include "led.h"
#include "sequencer.h"
#include "timer.h"

static enum DisplayMode display_mode = SEQ;
static volatile uint8_t led_count = 0;
static volatile long last_led_count_tick = 0;

static int g_led_i = 0;

static uint8_t led_idx_to_port_and_ddr[16][2] = {
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
    PORTD |= led_idx_to_port_and_ddr[g_led_i][0];
    DDRD   = led_idx_to_port_and_ddr[g_led_i][1];
  }
}

void output_led_on_value() {
  for (int i =0; i < led_count; ++i) {
    PORTD |= led_idx_to_port_and_ddr[i][0];
    DDRD   = led_idx_to_port_and_ddr[i][1];
  }
}

// led_task
// charlieplexing
// use PD0 - PD5
void output_led() {
  unsigned long duration;

  DDRD = 0;
  PORTD &= (uint8_t)~LED_MASK;
  switch (display_mode) {
    case SEQ:
      output_led_on_seq();
      break;
    case COUNT:
      duration = ticks() - last_led_count_tick;
      if (duration > MAX_DISPLAY_TICK_FOR_VALUE || duration < 0 /* count wrap? */) {
        display_mode = SEQ;
        output_led_on_seq();
      } else {
        output_led_on_value();
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
}

void set_led_count(uint8_t count /* value: 1..16 */, uint8_t tick) {
  display_mode = COUNT;
  led_count = count;
  last_led_count_tick = tick;
}
