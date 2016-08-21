#include "pitch_cycle.h"
#include <avr/pgmspace.h>

// C1 - B1
const uint16_t pitch_to_cycle[12] PROGMEM = {
  7645,
  7215,
  6810,
  6428,
  6067,
  5727,
  5405,
  5102,
  4816,
  4545,
  4290,
  4050
};
