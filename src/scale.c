#include <avr/pgmspace.h> 
#include "scale.h"

extern const char scale_table[16][12] PROGMEM = {
// C,C#, D,D#, E, F,F#, G,G#, A,A#, B
// 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11
  // one
  {0, 0, 0, 0, 0, 0,12,12,12,12,12,12},
  // 5 th
  {0, 0, 0, 0, 7, 7, 7, 7, 7, 7,12,12},
  // 2th 5th
  {0, 0, 2, 2, 2, 7, 7, 7, 7, 7,12,12},
  // 2th 5th 6th
  {0, 0, 2, 2, 2, 7, 7, 7, 9, 9, 9,12},
  // pentatonic 
  {0, 0, 2, 2, 4, 4, 4, 7, 7, 9, 9,12},
  // 6 voice scale
  {0, 0, 2, 2, 4, 4, 7, 7, 9, 9,11,11},
  // diatonic
  {0, 0, 2, 2, 4, 5, 5, 7, 7, 9, 9,11},
  // harmonic
  {0, 0, 2, 2, 4, 5, 5, 7, 7, 8, 8,11},
  // melodic
  {0, 0, 2, 2, 4, 5, 5, 6, 6, 8, 8,11},
  // gamlan
  {0, 1, 1, 3, 3, 7, 7, 7, 8, 8,12,12},
  // chinese
  {0, 0, 0, 3, 3, 6, 6, 7, 7,11,11,11},
  // augmented 
  {0, 0, 3, 3, 4, 4, 7, 7, 8, 8,11,11},
  // ?
  {0, 0, 3, 3, 4, 4, 6, 6, 9, 9,11,11},
  // diminish
  {0, 2, 2, 3, 3, 5, 6, 6, 8, 9, 9,11},
  // blues
  {0, 2, 2, 3, 3, 5, 6, 7, 8, 9, 9,10},
  // chromatic
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11}
};
