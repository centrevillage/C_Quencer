#include <avr/io.h>

#ifndef CQ_PATTERN_H_
#define CQ_PATTERN_H_

#define WAVETABLE_TYPE_SIZE 1
#define WAVETABLE_SIZE 256

// TODO: many wave table!
extern const uint8_t wavetables[WAVETABLE_TYPE_SIZE][WAVETABLE_SIZE];
extern const uint8_t selected_wavetable_type;

#endif /* CQ_PATTERN_H_ */
