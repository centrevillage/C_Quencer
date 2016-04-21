#include "dac.h"
#include "input.h"
#include "sequencer.h"
#include "wavetable.h"
#include "slide_table.h"

#include <avr/pgmspace.h>

// ピッチを1 tick(timer1) = (16us) あたりのtable index移動量として表現
// C1 - B8
static const float pitch_to_table_index[120] PROGMEM = {
  0.1339522894339065,
  0.14191750706015852,
  0.15035636117371268,
  0.15929701566570542,
  0.1687693091393894,
  0.17880485449369798,
  0.18943714442835682,
  0.20070166322265562,
  0.2126360051609319,
  0.22528,
  0.23867584589726204,
  0.25286825024313553,
  0.267904578867813,
  0.28383501412031703,
  0.30071272234742535,
  0.31859403133141084,
  0.3375386182787788,
  0.35760970898739597,
  0.37887428885671365,
  0.40140332644531124,
  0.4252720103218638,
  0.45056,
  0.4773516917945241,
  0.505736500486271,
  0.535809157735626,
  0.5676700282406341,
  0.6014254446948506,
  0.6371880626628217,
  0.6750772365575577,
  0.7152194179747919,
  0.7577485777134273,
  0.8028066528906226,
  0.8505440206437276,
  0.90112,
  0.9547033835890482,
  1.011473000972542,
  1.071618315471252,
  1.1353400564812681,
  1.2028508893897012,
  1.2743761253256434,
  1.3501544731151154,
  1.4304388359495839,
  1.5154971554268546,
  1.6056133057812452,
  1.7010880412874552,
  1.80224,
  1.9094067671780959,
  2.0229460019450847,
  2.143236630942504,
  2.2706801129625362,
  2.4057017787794033,
  2.5487522506512867,
  2.70030894623023,
  2.860877671899168,
  3.030994310853709,
  3.2112266115624895,
  3.4021760825749108,
  3.60448,
  3.8188135343561918,
  4.045892003890169,
  4.286473261885008,
  4.5413602259250725,
  4.8114035575588066,
  5.0975045013025735,
  5.40061789246046,
  5.721755343798336,
  6.061988621707418,
  6.422453223124979,
  6.8043521651498216,
  7.20896,
  7.6376270687123835,
  8.091784007780339,
  8.572946523770016,
  9.082720451850145,
  9.622807115117613,
  10.195009002605147,
  10.80123578492092,
  11.443510687596673,
  12.123977243414837,
  12.844906446249958,
  13.608704330299643,
  14.41792,
  15.275254137424767,
  16.183568015560677,
  17.14589304754003,
  18.16544090370029,
  19.245614230235226,
  20.390018005210294,
  21.60247156984184,
  22.887021375193346,
  24.247954486829673,
  25.689812892499916,
  27.217408660599286,
  28.83584,
  30.550508274849555,
  32.36713603112134,
  34.29178609508006,
  36.330881807400594,
  38.49122846047043,
  40.78003601042059,
  43.20494313968371,
  45.77404275038667,
  48.49590897365935,
  51.37962578499987,
  54.434817321198544,
  57.67168,
  61.10101654969911,
  64.73427206224268,
  68.58357219016013,
  72.66176361480119,
  76.98245692094086,
  81.56007202084118,
  86.40988627936741,
  91.54808550077334,
  96.9918179473187,
  102.75925156999973,
  108.86963464239709,
  115.34336,
  122.20203309939822,
  129.46854412448536
};

volatile static uint8_t phase_shift = 0;

void spi_init() {
  //Enable SPI, Master, set clock rate fck/64 = 250kHz
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1);
  SPSR = 0x00;
}

static inline uint8_t trans_spi(uint8_t data) {
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

static uint16_t prev_current_table_idx = 0;
volatile uint8_t is_request_update_phase_shift = 0;

//C0- B9 = 0 - 119
//A5(69) = 440
//1 timer count = 62.5kHz = 1/ 62500 s = 16 us / 1 cycle
// ldac_pin = PB1
// ss_pin = PB0
void output_osc(uint16_t timer_count) {
  if (is_request_update_phase_shift) {
    phase_shift = prev_current_table_idx;
    is_request_update_phase_shift = 0;
  }
  float current_idx = pgm_read_float(&(pitch_to_table_index[current_pitch]));
  if (current_values.v.slide > 0 && prev_pitch < 120 && active_seq[current_step]) {
    float prev_idx = pgm_read_float(&(pitch_to_table_index[prev_pitch]));
    if (prev_idx != current_idx) {
      uint16_t slide_count = (prev_idx < current_idx) ? ((uint16_t)current_values.v.slide * 32) : ((uint16_t)current_values.v.slide * 48);
      if (slide_count > timer_count) {
        float rate = ((float)timer_count/(float)slide_count);
        rate = pgm_read_float(&(slide_table[(uint8_t)(rate * 256)]));
        current_idx = (current_idx - prev_idx) * rate + prev_idx;
      }
    }
  }
  uint16_t current_table_index = ((uint16_t)(current_idx*timer_count+phase_shift)%WAVETABLE_SIZE);
  uint16_t current_value = pgm_read_word(&(wavetables[selected_wavetable_type][current_table_index]));
  output_dac_a(current_value);
  prev_current_table_idx = current_table_index;
}

void output_cv(uint16_t timer_count) {
  // current_pitch = N pitch unit; 1 octave = 12 pitch unit
  // 128 pitch unit = 4096 dac value = 5 V pin out
  // 120 pitch unit = 5 * 120 / 128  = 4.68 V pin out
  // 4.68 V pin out -> analog gain x 2.17 -> 10V cv out
  output_dac_b(((uint16_t)current_pitch)*32);
}

void reset_phase_shift() {
  phase_shift = 0;
}

void update_phase_shift() {
  is_request_update_phase_shift = 1;
}
