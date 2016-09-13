#include "pitch_cycle.h"
#include <avr/pgmspace.h>

// C1 - B1
const uint16_t pitch_to_cycle[12] PROGMEM = {
  30578,
  28862,
  27242,
  25713,
  24270,
  22908,
  21622,
  20408,
  19263,
  18182,
  17161,
  16198
};
