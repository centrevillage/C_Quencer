#include "led.h"
#include "sequencer.h"

static int g_led_i = 0;

// led_task
// charlieplexing
// use PD0 - PD5
void output_led() {
  DDRD = 0;
  PORTD &= (uint8_t)~LED_MASK;
  if (active_seq[g_led_i]) {
    switch (g_led_i) {
      case 0:
        PORTD |= 0b00000010;
        DDRD  = 0b00000011;
        break;
      case 1:
        PORTD |= 0b00000001;
        DDRD  = 0b00000011;
        break;
      case 2:
        PORTD |= 0b00000100;
        DDRD  = 0b00000101;
        break;
      case 3:
        PORTD |= 0b00000001;
        DDRD  = 0b00000101;
        break;
      case 4:
        PORTD |= 0b00000100;
        DDRD  = 0b00000110;
        break;
      case 5:
        PORTD |= 0b00000010;
        DDRD  = 0b00000110;
        break;
      case 6:
        PORTD |= 0b00001000;
        DDRD  = 0b00001010;
        break;
      case 7:
        PORTD |= 0b00000010;
        DDRD  = 0b00001010;
        break;
      case 8:
        PORTD |= 0b00001000;
        DDRD  = 0b00001100;
        break;
      case 9:
        PORTD |= 0b00000100;
        DDRD  = 0b00001100;
        break;
      case 10:
        PORTD |= 0b00010000;
        DDRD  = 0b00010100;
        break;
      case 11:
        PORTD |= 0b00000100;
        DDRD  = 0b00010100;
        break;
      case 12:
        PORTD |= 0b00010000;
        DDRD  = 0b00011000;
        break;
      case 13:
        PORTD |= 0b00001000;
        DDRD  = 0b00011000;
        break;
      case 14:
        PORTD |= 0b00010000;
        DDRD  = 0b00010001;
        break;
      case 15:
        PORTD |= 0b00000001;
        DDRD  = 0b00010001;
        break;
      default:
        break;
    }
  }
  if (++g_led_i >= 16) {
    g_led_i = 0;
  }
}
