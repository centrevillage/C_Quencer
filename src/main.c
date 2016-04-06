// CV OUT, OSC OUT 以外の確認はOK
#include "pins_arduino.h"
#include <SPI.h>

const int knob1_pin = 0;
const int knob2_pin = 1;
const int knob3_pin = 2;
const int knob4_pin = 3;
const int ss_pin = 8;
const int ldac_pin = 9;
const int LDAC_MASK      = B00000010; // PB1
const int SS_MASK        = B00000001; // PB0
const int LDAC_SS_MASK   = B00000011;
const int switch1_pin = 5;
const int switch2_pin = 6;
const int switch3_pin = 7;
const int switch4_pin = A5;
const int trig_in_pin = A4;
const int gate_out_pin = 10;

unsigned char g_active_seq[16];

struct Task {
  unsigned long interval;
  unsigned long last;
  void (*callback)();
};

const int g_tasks_len = 4;
struct Task g_tasks[g_tasks_len];

// seq task
int g_step_i = 0;
void step_seq() {
  g_active_seq[g_step_i] = 1;
  if (++g_step_i >= 16) {
    g_step_i = 0;
    memset(g_active_seq, 0, sizeof(g_active_seq));
  }
}

// input task
float g_fill_len_rate = 0.0f;
void handle_input() {
  int fill_len_v = analogRead(knob1_pin);
  int rot_v = analogRead(knob2_pin);
  int seqpat_v = analogRead(knob3_pin);
  int range_v = analogRead(knob4_pin);
  int func_b = digitalRead(switch1_pin);
  int start_b = digitalRead(switch2_pin);
  int tap_b = digitalRead(switch3_pin);
  int rec_b = digitalRead(switch4_pin);

  g_fill_len_rate = (float) fill_len_v / 1024.0f;
}

// led_task
// charlieplexing
// use PD0 - PD5
int g_led_i = 0;
const unsigned char LED_MASK = B00011111;
const unsigned char LED_MASK_R = ~LED_MASK;
void output_led() {
  DDRD = 0;
  PORTD &= LED_MASK_R;
  if (g_active_seq[g_led_i]) {
    switch (g_led_i) {
      case 0:
        PORTD |= B00000010;
        DDRD  = B00000011;
        break;
      case 1:
        PORTD |= B00000001;
        DDRD  = B00000011;
        break;
      case 2:
        PORTD |= B00000100;
        DDRD  = B00000101;
        break;
      case 3:
        PORTD |= B00000001;
        DDRD  = B00000101;
        break;
      case 4:
        PORTD |= B00000100;
        DDRD  = B00000110;
        break;
      case 5:
        PORTD |= B00000010;
        DDRD  = B00000110;
        break;
      case 6:
        PORTD |= B00001000;
        DDRD  = B00001010;
        break;
      case 7:
        PORTD |= B00000010;
        DDRD  = B00001010;
        break;
      case 8:
        PORTD |= B00001000;
        DDRD  = B00001100;
        break;
      case 9:
        PORTD |= B00000100;
        DDRD  = B00001100;
        break;
      case 10:
        PORTD |= B00010000;
        DDRD  = B00010100;
        break;
      case 11:
        PORTD |= B00000100;
        DDRD  = B00010100;
        break;
      case 12:
        PORTD |= B00010000;
        DDRD  = B00011000;
        break;
      case 13:
        PORTD |= B00001000;
        DDRD  = B00011000;
        break;
      case 14:
        PORTD |= B00010000;
        DDRD  = B00010001;
        break;
      case 15:
        PORTD |= B00000001;
        DDRD  = B00010001;
        break;
      default:
        break;
    }
  }
  if (++g_led_i >= 16) {
    g_led_i = 0;
  }
}

// DAC task
// ldac_pin = PB1
// ss_pin = PB0
void output_osc() {
  //// test
  //int max_i = 4096 * g_fill_len_rate;
  //for (int i = 0; i < max_i; i += 4) {
  //  //digitalWrite(ldac_pin, HIGH);
  //  //digitalWrite(ss_pin, LOW);
  //  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;

  //  SPI.transfer((i >> 8) | 0x30);
  //  SPI.transfer(i & 0xff);

  //  //digitalWrite(ss_pin, HIGH);
  //  //digitalWrite(ldac_pin, LOW);
  //  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
  //}
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize pin
  for (int i = 0; i < 5; ++i) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  pinMode(switch1_pin, INPUT_PULLUP);
  pinMode(switch2_pin, INPUT_PULLUP);
  pinMode(switch3_pin, INPUT_PULLUP);
  pinMode(switch4_pin, INPUT_PULLUP);
  pinMode(ss_pin, OUTPUT);
  pinMode(ldac_pin, OUTPUT);

  // initialize sequence
  memset(g_active_seq, 0, sizeof(g_active_seq));

  // initialize DAC
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.setDataMode(SPI_MODE0);

  // setup task
  unsigned long start_usec = micros();
  struct Task seq_task = {50000, start_usec, step_seq},
              led_task = {10, start_usec, output_led},
              input_task = {10, start_usec, handle_input},
              dac_task = {10, start_usec, output_osc};
  g_tasks[0] = input_task;
  g_tasks[1] = led_task;
  g_tasks[2] = seq_task;
  g_tasks[3] = dac_task;
}

unsigned long g_current_usec = 0;
void loop() {
  const unsigned long usec = micros();
  if (g_current_usec > usec) {
    // time wrap!
    for (int i = 0; i < g_tasks_len; ++i) {
      struct Task * const t = &g_tasks[i];
      (*t).last = usec;
    }
  }
  g_current_usec = usec;
  for (int i = 0; i < g_tasks_len; ++i) {
    struct Task * const t = &g_tasks[i];
    if (usec - (*t).last > (*t).interval) {
      (*t).last = usec;
      (*t).callback();
    }
  }
}
