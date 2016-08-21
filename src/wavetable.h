#include <avr/io.h>

#ifndef CQ_WAVETABLE_H_
#define CQ_WAVETABLE_H_

#define WAVETABLE_TYPE_SIZE 4
#define WAVETABLE_SIZE 1024

extern const uint16_t wavetables[WAVETABLE_TYPE_SIZE][WAVETABLE_SIZE];

#endif /* CQ_WAVETABLE_H_ */
