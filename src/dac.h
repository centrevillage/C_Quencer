#include <avr/io.h>

#ifndef CQ_DAC_H_
#define CQ_DAC_H_

#define LDAC_MASK 0b00000010
#define SS_MASK   0b00000001
#define LDAC_SS_MASK 0b00000011
#define BASE_HZ 440

void output_osc(uint16_t timer_count);
void output_cv(uint16_t timer_count);
void spi_init();

static inline uint8_t trans_spi(uint8_t data);

static const float pitch_to_table_index[120];

#endif /* CQ_DAC_H_ */
