#include <avr/io.h>
#include <avr/Interrupt.h>
#include <stdlib.h>

#ifndef CQ_INPUT_H_
#define CQ_INPUT_H_

union ControllerState {
  uint8_t data;
  struct {
    uint8_t rec :1;   // on recording
    uint8_t start :1; // on sequence running
    uint8_t func :1;   // on sub function
    uint8_t hid :1;   // on hidden function
    uint8_t func_lock :1; // on sub function lock
  } bit;

};
volatile union ControllerState current_state;

// scale most lo-bit = C, most high-bit = B
// 0x11111111 -> BAG(F#)FEDC
// scale value is 8bit boolean field.
// 255 overvalue as null value
struct ControllerValue {
  uint16_t step_fill;
  uint16_t step_length;
  uint16_t step_rot;
  uint16_t step_rand;
  uint16_t scale_select;
  uint16_t scale_select_random;
  uint16_t scale_transpose;
  uint16_t scale_range;
  uint16_t scale_pattern_random;
  uint16_t glide;
  uint16_t swing;
} current_values;

struct ControllerValue recorded_values[128];


volatile uint8_t knob_values[4];
volatile uint8_t button_state[4];

enum RecMode {STOP, REC, PLAY};
extern volatile enum RecMode rec_mode;

void read_knob_values();
void set_current_value(uint8_t value, uint8_t knob_idx);
uint8_t func_on();
uint8_t hid_on();


#endif /* CQ_INPUT_H_ */
