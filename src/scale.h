#include <avr/io.h>

#ifndef CQ_SCALE_H_
#define CQ_SCALE_H_

extern const uint16_t scale_defs[16];
extern volatile char scale_table[16][12];

void update_scale_table(uint16_t scales[16]);

#endif /* CQ_SCALE_H_ */
