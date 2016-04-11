#include <avr/io.h>
#include <avr/Interrupt.h>
#include <stdlib.h>

#ifndef CQ_INPUT_H_
#define CQ_INPUT_H_

enum RecMode {STOP, REC, PLAY};
extern volatile enum RecMode rec_mode;

enum FuncMode {NONE, FUNC, HID};
extern volatile enum FuncMode func_mode;

union ControllerState {
  uint8_t rec;   // on recording
  uint8_t start; // on sequence running
  uint8_t func;   // on sub function
  uint8_t hid;   // on hidden function
  uint8_t func_lock; // on sub function lock
  uint8_t data;
};

enum FuncMode get_func_mode();

// scale most lo-bit = C, most high-bit = B
// 0x11111111 -> BAG(F#)FEDC
// scale value is 8bit boolean field.
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

struct ButtonHistory {
  enum FuncMode mode; 
  uint8_t button_idx;
  uint8_t count;
  unsigned long last_tick;
  uint16_t interval_tick;
};

struct ControllerValue recorded_values[128];
struct ControllerValue recorded_value_flags[128]; // 0 = not recorded, 1 = recorded


volatile uint8_t knob_values[4];
volatile uint8_t button_state[4];

void read_knob_values();
void set_current_value(uint8_t value, uint8_t knob_idx);
uint8_t func_on();
uint8_t hid_on();
void press(uint8_t button_idx);


#endif /* CQ_INPUT_H_ */
