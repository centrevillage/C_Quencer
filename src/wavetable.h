#include <avr/io.h>
#include <avr/pgmspace.h>

#ifndef CQ_WAVETABLE_H_
#define CQ_WAVETABLE_H_

#define WAVETABLE_TYPE_SIZE 5
#define WAVETABLE_SIZE 1024

extern const uint16_t wavetables[WAVETABLE_TYPE_SIZE][WAVETABLE_SIZE];

inline uint16_t sine_wave(uint16_t index) {
  return pgm_read_word(&(wavetables[2][index]));
}

inline uint16_t log_wave(uint16_t index) {
  return pgm_read_word(&(wavetables[4][index]));
}

#endif /* CQ_WAVETABLE_H_ */
