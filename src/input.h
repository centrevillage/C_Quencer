#include <avr/io.h>
#include <avr/Interrupt.h>
#include <stdlib.h>
#include <string.h>

#ifndef CQ_INPUT_H_
#define CQ_INPUT_H_

enum RecMode {STOP, REC, PLAY};
extern volatile enum RecMode rec_mode;

enum FuncMode {NONE, FUNC, HID};
extern volatile enum FuncMode func_mode;

typedef struct {
  uint8_t rec;   // on recording
  uint8_t start; // on sequence running
  uint8_t func;   // on sub function
  uint8_t hid;   // on hidden function
  uint8_t func_lock; // on sub function lock
  uint8_t data;
} ControllerState;

enum FuncMode get_func_mode();

// scale most lo-bit = C, most high-bit = B
// 0x11111111 -> BAG(F#)FEDC
// scale value is 8bit boolean field.
typedef union {
  uint8_t values[12];
  struct {
    uint8_t step_fill;
    uint8_t step_length;
    uint8_t step_rot;
    uint8_t step_rand;
    uint8_t scale_select;
    uint8_t scale_shift;
    uint8_t scale_pattern;
    uint8_t scale_transpose;
    uint8_t scale_range;
    uint8_t scale_pattern_random;
    uint8_t slide;
    uint8_t swing;
  } v;
} ControllerValue;

extern volatile ControllerValue current_values;

typedef struct {
  enum FuncMode mode; 
  uint8_t button_idx;
  uint8_t count;
  unsigned long last_tick;
  uint16_t interval_tick;
} ButtonHistory;

extern volatile ControllerState current_state;
extern volatile ButtonHistory button_history;

volatile ControllerValue recorded_values[64];
volatile ControllerValue recorded_value_flags[64]; // 0 = not recorded, 1 = recorded
volatile ControllerValue changed_value_flags; // 0 = not changed, 1 = changed


volatile static uint8_t knob_values[4][8];
volatile static uint8_t button_state[4];

void read_knob_values();
void set_current_value(uint8_t value, uint8_t knob_idx);
void press(uint8_t button_idx);
void record_current_knob_values();
void start_recording();
void end_recording();
void clear_recording();
void play_recorded_knob_values();
void reset_all_input();


#endif /* CQ_INPUT_H_ */
