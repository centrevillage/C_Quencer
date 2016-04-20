#include "dac.h"
#include "input.h"
#include "sequencer.h"
#include "wavetable.h"
#include "slide_table.h"

#include <avr/pgmspace.h>

// ピッチを1 tick(timer1) = (16us) あたりのtable index移動量として表現
// C1 - B8
static const float pitch_to_table_index[120] PROGMEM = {
  0.033488072358476624,
  0.03547937676503962,
  0.03758909029342817,
  0.039824253916426355,
  0.04219232728484735,
  0.044701213623424496,
  0.0473592861070892,
  0.050175415805663905,
  0.053159001290232974,
  0.056319999999999995,
  0.05966896147431551,
  0.06321706256078388,
  0.06697614471695325,
  0.07095875353007924,
  0.07517818058685634,
  0.07964850783285271,
  0.0843846545696947,
  0.08940242724684899,
  0.0947185722141784,
  0.10035083161132781,
  0.10631800258046595,
  0.11263999999999999,
  0.11933792294863102,
  0.12643412512156774,
  0.1339522894339065,
  0.1419175070601585,
  0.15035636117371265,
  0.15929701566570542,
  0.16876930913938942,
  0.17880485449369798,
  0.1894371444283568,
  0.20070166322265565,
  0.2126360051609319,
  0.22527999999999998,
  0.23867584589726204,
  0.2528682502431355,
  0.267904578867813,
  0.283835014120317,
  0.3007127223474253,
  0.31859403133141084,
  0.33753861827877885,
  0.35760970898739597,
  0.3788742888567136,
  0.4014033264453113,
  0.4252720103218638,
  0.45055999999999996,
  0.47735169179452397,
  0.5057365004862712,
  0.535809157735626,
  0.567670028240634,
  0.6014254446948508,
  0.6371880626628217,
  0.6750772365575575,
  0.715219417974792,
  0.7577485777134272,
  0.8028066528906224,
  0.8505440206437277,
  0.9011199999999999,
  0.9547033835890479,
  1.0114730009725423,
  1.071618315471252,
  1.135340056481268,
  1.2028508893897016,
  1.2743761253256434,
  1.350154473115115,
  1.430438835949584,
  1.5154971554268544,
  1.6056133057812447,
  1.7010880412874554,
  1.8022399999999998,
  1.9094067671780959,
  2.0229460019450847,
  2.143236630942504,
  2.270680112962536,
  2.4057017787794033,
  2.5487522506512867,
  2.70030894623023,
  2.860877671899168,
  3.0309943108537087,
  3.2112266115624895,
  3.4021760825749108,
  3.6044799999999997,
  3.8188135343561918,
  4.045892003890169,
  4.286473261885008,
  4.541360225925072,
  4.8114035575588066,
  5.0975045013025735,
  5.40061789246046,
  5.721755343798336,
  6.0619886217074175,
  6.422453223124979,
  6.8043521651498216,
  7.208959999999999,
  7.637627068712389,
  8.091784007780335,
  8.572946523770016,
  9.082720451850147,
  9.622807115117608,
  10.195009002605147,
  10.801235784920927,
  11.443510687596667,
  12.123977243414835,
  12.844906446249967,
  13.608704330299636,
  14.417919999999999,
  15.275254137424778,
  16.18356801556067,
  17.14589304754003,
  18.165440903700294,
  19.245614230235216,
  20.390018005210294,
  21.602471569841853,
  22.887021375193335,
  24.24795448682967,
  25.689812892499933,
  27.217408660599272,
  28.835839999999997,
  30.550508274849555,
  32.36713603112134
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

//C0- B9 = 0 - 119
//A5(69) = 440
//1 timer count = 62.5kHz = 1/ 62500 s = 16 us / 1 cycle
// ldac_pin = PB1
// ss_pin = PB0
void output_osc(uint16_t timer_count) {
  float current_idx = pgm_read_float(&(pitch_to_table_index[current_pitch]));
  if (current_values.v.slide > 0 && prev_pitch < 120 && active_seq[current_step]) {
    float prev_idx = pgm_read_float(&(pitch_to_table_index[prev_pitch]));
    if (prev_idx != current_idx) {
      uint16_t slide_count = (prev_idx < current_idx) ? ((uint16_t)current_values.v.slide * 32) : ((uint16_t)current_values.v.slide * 48);
      if (slide_count > timer_count) {
        float rate = ((float)timer_count/(float)slide_count);
        rate = pgm_read_float(&(slide_table[(uint8_t)(rate * WAVETABLE_SIZE)]));
        current_idx = (current_idx - prev_idx) * rate + prev_idx;
      }
    }
  }
  uint8_t current_table_index = (uint8_t)((uint16_t)(current_idx*timer_count+phase_shift)%WAVETABLE_SIZE);
  uint16_t current_value = pgm_read_byte(&(wavetables[selected_wavetable_type][current_table_index]));
  output_dac_a(current_value*16);
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

void update_phase_shift(uint16_t timer_count) {
  float current_idx = pgm_read_float(&(pitch_to_table_index[current_pitch]));
  phase_shift = (uint8_t)((uint16_t)(current_idx*timer_count+phase_shift)%WAVETABLE_SIZE);
}
