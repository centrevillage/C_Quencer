#include "dac.h"
#include "input.h"
#include "variables.h"
#include "wavetable.h"
#include "cycle_speed.h"
#include "sequencer.h"

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

inline void output_dac_a(uint16_t data) {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  trans_spi(((data >> 8) & 0x0F) | _BV(4) | _BV(5)); // OUT A, gain x1, no shutdown
  trans_spi(data & 0xFF);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}

inline void output_dac_b(uint16_t data) {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  trans_spi(((data >> 8) & 0x0F) | _BV(7) | _BV(4) | _BV(5)); // OUT B, gain x1, no shutdown
  trans_spi(data & 0xFF);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}


uint16_t overshoot_ticks = 0;
uint16_t overshoot_count_buf = 0;
uint8_t vibrato_ticks = 0;
uint16_t vibrato_count_buf = 0;
uint16_t phase_count_buf = 0;
uint16_t phase_ticks = 0;
uint16_t prev_phase_ticks = 0;
uint8_t phase_direction = 1;
volatile uint16_t wave1_count_in_cycle = 0;
volatile uint16_t wave2_count_in_cycle = 0;
volatile uint16_t current_table_index1 = 0;
volatile uint16_t current_table_index2 = 0;
const uint8_t shift_oct = 0;
volatile int8_t pitch_diff = 0;
volatile int8_t pitch_diff_abs = 0;
volatile int8_t log_table_idx_speed = 0;
inline void output_osc_and_cv_on_normal(uint8_t interval_count, uint8_t delta_tick){
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

  int16_t pitch_mod = 0;
  if (log_table_idx_speed) {
    uint16_t log_table_idx = (get_last_step_duration_ticks() * log_table_idx_speed) + 431;

    if (log_table_idx >= WAVETABLE_SIZE) {
      log_table_idx_speed = 0;
      vibrato_ticks = 0;
      vibrato_count_buf = 0;
    } else {
      uint16_t sine_table_idx = (log_wave(log_table_idx) - 3584) << 1;
      uint16_t divide = (sine_table_idx >> 4) + (24 - pitch_diff_abs) + 1;
      uint8_t mult = (no_rec_values.v.pitch_overshoot & 0x0F) + 1;
      if (!!(no_rec_values.v.pitch_overshoot & 0x10) ^ (pitch_diff < 0)) {
        // invert
        sine_table_idx = WAVETABLE_SIZE - sine_table_idx;
      }
      pitch_mod = (sine_wave(sine_table_idx) * mult / divide) - (2048 * mult / divide);
    }
  } else if (no_rec_values.v.pitch_vibrato) {
    vibrato_count_buf += delta_tick * (no_rec_values.v.pitch_vibrato & 0x0F) * 2;
    vibrato_ticks += vibrato_count_buf >> 8;
    vibrato_count_buf &= 0xFF;
    pitch_mod = (int16_t)(sine_wave(vibrato_ticks << 2) >> 4);
    if (!(no_rec_values.v.pitch_vibrato & 0x10)) {
      uint16_t step_duration = get_last_step_duration_ticks();
      if (step_duration < 0x7F) {
        uint8_t mod = (0x7F - step_duration) >> 4;
        if (mod) {
          pitch_mod /= mod;
          pitch_mod -= 128 / mod;
        } else {
          pitch_mod -= 128;
        }
      } else {
        pitch_mod -= 128;
      }
    } else {
      pitch_mod -= 128;
    }
  }

  uint16_t cv_pitch;
  uint8_t slide_oct1;
  uint16_t slide_note1;

  if (slide_speed > 0 && slide_pitch1 != current_pitch1_dec) { // pitch slide
    slide_pitch1 = ((uint32_t)slide_speed * current_pitch1_dec + (uint32_t)(256 - slide_speed) * slide_pitch1 + 128) / 256;
    cv_pitch = slide_pitch1;
    slide_oct1 = slide_pitch1 / (12*256);
    slide_note1 = slide_pitch1 % (12*256);
  } else {
    cv_pitch = current_pitch1_dec;
    slide_oct1 = current_oct1;
    slide_note1 = current_note_num1 * 256;
  }
  uint32_t val1 = pgm_read_word(&(cycle_speed_table[slide_note1]));
  if (pitch_mod) {
    val1 += pitch_mod;
  }
  slide_buf_value1 += (val1 * interval_count) << (slide_oct1 + shift_oct);
  wave1_count_in_cycle += (uint16_t)(slide_buf_value1 >> 12);
  slide_buf_value1 &= 0x00000FFF;
  current_table_index1 = ((uint32_t)wave1_count_in_cycle * WAVETABLE_SIZE) >> 16;

  if (current_values.v.wave_pitch_duration) {

    uint8_t slide_oct2;
    uint16_t slide_note2;
    if (slide_speed > 0 && slide_pitch2 != current_pitch2_dec) { // pitch slide
      slide_pitch2 = ((uint32_t)slide_speed * current_pitch2_dec + (uint32_t)(256 - slide_speed) * slide_pitch2 + 128) / 256;
      slide_oct2 = slide_pitch2 / (12*256);
      slide_note2 = slide_pitch2 % (12*256);
    } else {
      slide_oct2 = current_oct2;
      slide_note2 = current_note_num2 * 256;
    }
    uint32_t val2 = pgm_read_word(&(cycle_speed_table[slide_note2]));
    if (pitch_mod) {
      val2 += pitch_mod;
    }
    slide_buf_value2 += (val2 * interval_count) << (slide_oct2 + shift_oct);
    wave2_count_in_cycle += (uint16_t)(slide_buf_value2 >> 12);
    slide_buf_value2 &= 0x00000FFF;
    current_table_index2 = ((((uint32_t)wave2_count_in_cycle * WAVETABLE_SIZE) >> 16) + phase_shift2) % WAVETABLE_SIZE;
  } else {
    current_table_index2 = (current_table_index1 + phase_shift2) % WAVETABLE_SIZE;
  }

  uint16_t wave1_value = pgm_read_word(&(wavetables[selected_wavetable_type1][current_table_index1]));
  uint16_t wave2_value = pgm_read_word(&(wavetables[selected_wavetable_type2][selected_wavetable_type2_sign ? current_table_index2 : (WAVETABLE_SIZE - 1 - current_table_index2)]));

  uint16_t current_value = wave1_value * wave1_volume + wave2_value * wave2_volume;
  output_dac_a(current_value >> 3);

  // current_pitch(cv_pitch) = N pitch unit; 1 octave = 12 pitch unit
  // 128 pitch unit = 4096 dac value = 5 V pin out
  // 120 pitch unit = 5 * 120 / 128  = 4.68 V pin out
  // 4.68 V pin out -> analog gain x 2.17 -> 10V cv out
  output_dac_b((cv_pitch >> 3) + (3 << 5)); // +3 -> change 1V = 'C' to 'A'
}

inline void output_osc_and_cv_on_edit(uint8_t interval_count, uint8_t delta_tick){
  uint32_t val1 = pgm_read_word(&(cycle_speed_table[current_note_num1 * 256]));
  slide_buf_value1 += (val1 * interval_count) << (current_oct1 + shift_oct);
  wave1_count_in_cycle += (uint16_t)(slide_buf_value1 >> 12);
  slide_buf_value1 &= 0x00000FFF;
  current_table_index1 = ((uint32_t)wave1_count_in_cycle * WAVETABLE_SIZE) >> 16;
  uint16_t current_value = pgm_read_word(&(wavetables[selected_wavetable_type1][current_table_index1]));

  output_dac_a(current_value);
  output_dac_b((current_pitch1 + 3)*32); // +3 -> change 1V = 'C' to 'A'
}

//C0- B9 = 0 - 119
//A5(69) = 440
//1 timer count = 62.5kHz = 1/ 62500 s = 16 us / 1 cycle
// ldac_pin = PB1
// ss_pin = PB0
inline void output_osc_and_cv(uint8_t interval_count, uint8_t delta_tick) {
  switch(edit_mode) {
    case NORMAL:
      if (in_start_seq) {
        output_osc_and_cv_on_normal(interval_count, delta_tick);
      }
      break;
    case SCALE:
    case PATTERN:
      output_osc_and_cv_on_edit(interval_count, delta_tick);
      break;
    default:
      break;
  }
}

void reset_counts_at_active_step() {
  overshoot_ticks = 0;
  overshoot_count_buf = 0;
  if (no_rec_values.v.pitch_overshoot) {
    pitch_diff = current_pitch1 - prev_pitch1;
    pitch_diff_abs = pitch_diff < 0 ? -pitch_diff : pitch_diff;
    if (pitch_diff_abs > 24) {
      pitch_diff_abs = 24;
    }
    log_table_idx_speed = ((65535 - step_interval) >> 11) + 1;
  } else {
    log_table_idx_speed = 0;
  }
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

volatile uint8_t prev_count = 0;
volatile uint8_t div16_count_buf = 0;
void dac_process() {
  cli();
  uint8_t current_count = TCNT2;
  uint8_t interval_count = current_count - prev_count;
  if (interval_count > 0 ) {
    div16_count_buf += interval_count;
    uint8_t div8_interval = div16_count_buf / 8;
    div16_count_buf -= div8_interval * 8;
    output_osc_and_cv(interval_count, div8_interval);
    prev_count = current_count;
  }
  sei();
}
