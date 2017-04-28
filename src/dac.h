#include <avr/io.h>

#ifndef CQ_DAC_H_
#define CQ_DAC_H_

#define LDAC_MASK 0b00000010
#define SS_MASK   0b00000001
#define LDAC_SS_MASK 0b00000011

void spi_init();
void reset_counts_at_active_step();
void reset_phase();
void dac_process();

#endif /* CQ_DAC_H_ */
