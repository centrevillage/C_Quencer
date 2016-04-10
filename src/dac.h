#include <avr/io.h>

#ifndef CQ_DAC_H_
#define CQ_DAC_H_

#define LDAC_MASK 0b00000010
#define SS_MASK   0b00000001
#define LDAC_SS_MASK 0b00000011

void output_osc();
void spi_init();
void trans_spi(char data);
char read_spi();

#endif /* CQ_DAC_H_ */
