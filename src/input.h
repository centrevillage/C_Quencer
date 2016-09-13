#include <avr/io.h>
#include <avr/Interrupt.h>
#include <stdlib.h>
#include <string.h>
#include "adc.h"

#ifndef CQ_INPUT_H_
#define CQ_INPUT_H_

enum EditMode {NORMAL, SELECT, SCALE, PATTERN};

#define CHG_VAL_FLAG_STEP_FILL 0
#define CHG_VAL_FLAG_STEP_LENGTH 1
#define CHG_VAL_FLAG_STEP_ROT 2
#define CHG_VAL_FLAG_STEP_RAND 3
#define CHG_VAL_FLAG_SCALE_SELECT 4
#define CHG_VAL_FLAG_SCALE_SHIFT 5
#define CHG_VAL_FLAG_SCALE_PATTERN 6
#define CHG_VAL_FLAG_SCALE_TRANSPOSE 7
#define CHG_VAL_FLAG_SCALE_RANGE 8
#define CHG_VAL_FLAG_SCALE_PAT_RAND 9
#define CHG_VAL_FLAG_SLIDE 10
#define CHG_VAL_FLAG_SWING 11
#define CHG_VAL_FLAG_WAVE_SELECT 12
#define CHG_VAL_FLAG_WAVE_PHASE 13
#define CHG_VAL_FLAG_WAVE_BALANCE 14
#define CHG_VAL_FLAG_WAVE_PITCH_DURATION 15

#define KNOB_VALUES_SIZE 8
#define RECORDED_VALUES_SIZE 64

#define MULTI_TAP_MAX_INTERVAL 65535
#define MULTI_TAP_TIMEOUT_INTERVAL 262143L
#define FLUTTERING_INTERVAL 800
#define TAP_TEMPO_MIN_INTERVAL 2048

extern volatile enum EditMode edit_mode;

enum RecMode {STOP, REC, PLAY};
extern volatile enum RecMode rec_mode;

enum FuncMode {NONE, FUNC, HID, WAVE_SHAPE};
extern volatile enum FuncMode func_mode;

typedef struct {
  uint8_t rec;   // on recording
  uint8_t start; // on sequence running
  uint8_t func;   // on sub function
  uint8_t hid;   // on hidden function
  uint8_t wave_shape;   // on wave shape function
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
    uint8_t wave_select;
    uint8_t wave_phase;
    uint8_t wave_balance;
    uint8_t wave_pitch_duration;
  } v;
} ControllerValue;

extern volatile ControllerValue current_values;

typedef struct {
  enum FuncMode mode; 
  uint8_t button_idx;
  uint8_t count;
  unsigned long last_tick;
  unsigned long interval_tick;
  unsigned long last_leave;
} ButtonHistory;

typedef struct {
  uint8_t knob_idx;
  uint8_t count;
  unsigned long last_tick;
  unsigned long interval_tick;
} KnobHistory;

extern volatile ControllerState current_state;
extern volatile ButtonHistory button_history;
extern volatile KnobHistory knob_history;

volatile ControllerValue recorded_values[RECORDED_VALUES_SIZE];
volatile uint16_t changed_value_flags; // 0 = not changed, 1 = changed

static volatile uint8_t button_state[4];

extern volatile uint8_t edit_preset_num;
extern volatile uint8_t edit_pos;
extern volatile uint16_t edit_scale;
extern volatile uint8_t edit_pattern[16];

extern volatile uint8_t knob_values[4][KNOB_VALUES_SIZE];
extern volatile uint8_t current_knob_idx;
extern volatile uint8_t current_knob_value_idx;

inline void read_knob_values() {
  knob_values[current_knob_idx][current_knob_value_idx] = 255 - adc_read(current_knob_idx);
  ++current_knob_idx;
  if (current_knob_idx > 3) {
    current_knob_idx = 0;
    current_knob_value_idx = (current_knob_value_idx + 1) % KNOB_VALUES_SIZE;
  }
}

void update_knob_values();
void set_current_value(uint8_t value, uint8_t knob_idx);
void set_current_value_on_normal(uint8_t value, uint8_t knob_idx);
void set_current_value_on_select(uint8_t value, uint8_t knob_idx);
void set_current_value_on_scale(uint8_t value, uint8_t knob_idx);
void set_current_value_on_pattern(uint8_t value, uint8_t knob_idx);
void press(uint8_t button_idx);
void press_on_normal(uint8_t button_idx);
void press_on_select(uint8_t button_idx);
void press_on_scale(uint8_t button_idx);
void press_on_pattern(uint8_t button_idx);
void record_current_knob_values();
void fill_remains_records(uint8_t quantized_len);
void start_recording();
void end_recording();
void next_play_pos();
void play_recorded_knob_values();
void reset_all_input();

void enter_edit_select_mode();
void leave_edit_select_mode();
void enter_edit_scale_mode();
void enter_edit_pattern_mode();

#endif /* CQ_INPUT_H_ */
