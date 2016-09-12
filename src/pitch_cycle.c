#include "pitch_cycle.h"
#include <avr/pgmspace.h>

// C1 - B1
const uint16_t pitch_to_cycle[12] PROGMEM = {
  61156,
  57724,
  54484,
  51426,
  48540,
  45815,
  43244,
  40817,
  38526,
  36364,
  34323,
  32396
};
