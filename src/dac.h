#include <avr/io.h>

#ifndef CQ_DAC_H_
#define CQ_DAC_H_

#define LDAC_MASK 0b00000010
#define SS_MASK   0b00000001
#define LDAC_SS_MASK 0b00000011

void output_osc_and_cv(uint8_t interval_count, uint8_t delta_tick);
void spi_init();
void reset_count_in_cycle();
void reset_phase();

#endif /* CQ_DAC_H_ */
