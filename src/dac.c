#include "dac.h"
#include "input.h"
#include "variables.h"
#include "wavetable.h"
#include "pitch_cycle.h"
#include "cycle_speed.h"

#include <avr/pgmspace.h>

void spi_init() {
  //Enable SPI, Master, set clock rate fck/2 = 8MHz
  SPCR = (1<<SPE)|(1<<MSTR);
  SPSR = (1<<SPI2X);
}

inline uint8_t trans_spi(uint8_t data) {
  SPDR = data;
  loop_until_bit_is_set(SPSR, SPIF);
  data = SPDR;
  return(data);
}

void output_dac_a(uint16_t data) {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  trans_spi(((data >> 8) & 0x0F) | _BV(4) | _BV(5)); // OUT A, gain x1, no shutdown
  trans_spi(data & 0xFF);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}

void output_dac_b(uint16_t data) {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  trans_spi(((data >> 8) & 0x0F) | _BV(7) | _BV(4) | _BV(5)); // OUT B, gain x1, no shutdown
  trans_spi(data & 0xFF);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}

//C0- B9 = 0 - 119
//A5(69) = 440
//1 timer count = 62.5kHz = 1/ 62500 s = 16 us / 1 cycle
// ldac_pin = PB1
// ss_pin = PB0
void output_osc_and_cv(uint16_t interval_count, uint8_t delta_tick) {
  switch(edit_mode) {
    case NORMAL:
      output_osc_and_cv_on_normal(interval_count, delta_tick);
      break;
    case SCALE:
    case PATTERN:
      output_osc_and_cv_on_edit(interval_count, delta_tick);
      break;
    default:
      break;
  }
}


uint16_t phase_count_buf = 0;
volatile uint16_t phase_ticks = 0;
volatile uint16_t prev_phase_ticks = 0;
volatile uint8_t phase_direction = 1;
volatile uint16_t wave1_count_in_cycle = 0;
volatile uint16_t wave2_count_in_cycle = 0;
volatile uint16_t current_table_index1 = 0;
volatile uint16_t current_table_index2 = 0;
void output_osc_and_cv_on_normal(uint16_t interval_count, uint8_t delta_tick){
  cli();
  uint16_t cv_pitch = current_pitch1_dec;

  uint16_t cycle_length1 = pgm_read_word(&(pitch_to_cycle[current_note_num1]));
  uint16_t cycle_length2 = pgm_read_word(&(pitch_to_cycle[current_note_num2]));

  if (slide_speed > 0 && slide_pitch1 != current_pitch1_dec) { // pitch slide
    slide_pitch1 = ((uint32_t)slide_speed * current_pitch1_dec + (uint32_t)(256 - slide_speed) * slide_pitch1 + 128) / 256;
    slide_pitch2 = ((uint32_t)slide_speed * current_pitch2_dec + (uint32_t)(256 - slide_speed) * slide_pitch2 + 128) / 256;
    cv_pitch = slide_pitch1;
    uint8_t slide_oct1 = slide_pitch1 / (12*256);
    uint16_t slide_note1 = slide_pitch1 % (12*256);
    uint8_t slide_oct2 = slide_pitch2 / (12*256);
    uint16_t slide_note2 = slide_pitch2 % (12*256);
    uint32_t val1 = pgm_read_word(&(cycle_speed_table[slide_note1]));
    uint32_t val2 = pgm_read_word(&(cycle_speed_table[slide_note2]));
    slide_buf_value1 += (val1 * interval_count) << slide_oct1;
    slide_buf_value2 += (val2 * interval_count) << slide_oct2;
    uint16_t slide_buf_value2_count = slide_buf_value2 >> 12;
    wave1_count_in_cycle = (wave1_count_in_cycle + (slide_buf_value1 >> 12)) % 7645;
    wave2_count_in_cycle = (wave2_count_in_cycle + (slide_buf_value2 >> 12)) % 7645;
    slide_buf_value1 &= 0x00000FFF;
    slide_buf_value2 &= 0x00000FFF;
  } else {
    wave1_count_in_cycle = (wave1_count_in_cycle + (interval_count << current_oct1)) % cycle_length1;
    wave2_count_in_cycle = (wave2_count_in_cycle + (interval_count << current_oct2)) % cycle_length2;
  }

  current_table_index1 = (uint32_t)wave1_count_in_cycle * WAVETABLE_SIZE / cycle_length1;

  uint16_t phase_shift2;
  if (wave_phase_shift_cycle > 0) {
    phase_count_buf += delta_tick << (wave_phase_shift_cycle-1);
    uint16_t tmp_phase_count = phase_count_buf >> 4;
    phase_ticks += tmp_phase_count;
    phase_count_buf &= 0x0F;
    if (prev_phase_ticks > phase_ticks) {
      phase_direction = -phase_direction;
    }
    if (phase_direction > 0) {
      phase_shift2 = phase_ticks/(0xFFFF/(WAVETABLE_SIZE));
    } else {
      phase_shift2 = WAVETABLE_SIZE - phase_ticks/(0xFFFF/WAVETABLE_SIZE);
    }
  } else {
    phase_shift2 = wave_phase_shift * (WAVETABLE_SIZE / 16);
  }
  current_table_index2 = (((uint32_t)wave2_count_in_cycle * WAVETABLE_SIZE / cycle_length2) + phase_shift2) % WAVETABLE_SIZE;

  uint16_t wave1_value = pgm_read_word(&(wavetables[selected_wavetable_type1][current_table_index1]));
  uint16_t wave2_value = pgm_read_word(&(wavetables[selected_wavetable_type2 & 0x03][(selected_wavetable_type2 & 0x04) ? (WAVETABLE_SIZE - 1 - current_table_index2) : current_table_index2]));

  uint16_t current_value = wave1_value * wave1_volume + wave2_value * wave2_volume;
  output_dac_a(current_value >> 3);

  // current_pitch(cv_pitch) = N pitch unit; 1 octave = 12 pitch unit
  // 128 pitch unit = 4096 dac value = 5 V pin out
  // 120 pitch unit = 5 * 120 / 128  = 4.68 V pin out
  // 4.68 V pin out -> analog gain x 2.17 -> 10V cv out
  output_dac_b(cv_pitch >> 3);
  sei();
}

void output_osc_and_cv_on_edit(uint16_t interval_count, uint8_t delta_tick){
  uint16_t cycle_length1 = pgm_read_word(&(pitch_to_cycle[current_note_num1]));
  wave1_count_in_cycle = (wave1_count_in_cycle + (interval_count << current_oct1)) % cycle_length1;
  current_table_index1 = (uint32_t)wave1_count_in_cycle * WAVETABLE_SIZE / cycle_length1;
  uint16_t current_value = pgm_read_word(&(wavetables[selected_wavetable_type1][current_table_index1]));

  output_dac_a(current_value);
  output_dac_b(current_pitch1*32);
}

void reset_count_in_cycle() {
  uint16_t cycle_length1 = pgm_read_word(&(pitch_to_cycle[current_note_num1]));
  uint16_t cycle_length2 = pgm_read_word(&(pitch_to_cycle[current_note_num2]));
  wave1_count_in_cycle = (uint32_t)current_table_index1 * cycle_length1 / WAVETABLE_SIZE;
  wave2_count_in_cycle = (uint32_t)current_table_index2 * cycle_length2 / WAVETABLE_SIZE;
}

void reset_phase() {
  prev_phase_ticks = 0;
  phase_count_buf = 0;
  phase_ticks = 0;
  wave1_count_in_cycle = 0;
  wave2_count_in_cycle = 0;
  current_table_index1 = 0;
  current_table_index2 = 0;
}

