#include <avr/pgmspace.h> 
#include "scale.h"

const uint16_t scale_defs[16] PROGMEM = {
  // 0b0000111111111111
  // B A# A G# G F# F E D# D C# C
  // one
  0b0000000000000001,
  // 5 th
  0b0000000010000001,
  // 2th 5th
  0b0000000010000101,
  // 2th 5th 6th
  0b0000001010000101,
  // pentatonic 
  0b0000001010010101,
  // 6 voice scale
  0b0000101010010101,
  // diatonic
  0b0000101010110101,
  // harmonic
  0b0000100110110101,
  // melodic
  0b0000100101110101,
  // gamlan
  0b0000000110001011,
  // chinese
  0b0000100011001001,
  // augmented 
  0b0000100110011001,
  // ?
  0b0000101001011001,
  // diminish
  0b0000101101101101,
  // blues
  0b0000011111101101,
  // chromatic
  0b0000111111111111
};

char is_scale_table_complete(char scale_table_works[24]) {
  for (char i=6; i<18; ++i) {
    if (scale_table_works[i] == -128) {
      return 0;
    }
  }
  return 1; 
}

// 可能な限り均等にスケールノートをテーブルに配置
void update_scale_table(uint16_t scales[16]) {
  char scale_table_works[24]; // F0#-B0(0-5), C1-B9(6-17), C2-F2(18-23)
  for (char i=0; i<16; ++i) {
    memset(&scale_table_works, -128, sizeof(scale_table_works));
    char found = 0;
    for (char j=0; j<12; ++j) {
      if (scales[i] & (1<<j)) {
        found = 1;
        scale_table_works[j+6] = j;
        if (j < 6) {
          scale_table_works[j+18] = j+12;
        } else {
          scale_table_works[j-6] = j-12;
        }
      }
    }
    if (!found) {
      memset(&scale_table_works, 0, 24);
    }

    while(!is_scale_table_complete(scale_table_works)) {
      // 各ノートについて左右どちらかに空きがあれば埋めていく
      for (char j=0; j<24; ++j) {
        if (scale_table_works[j] != -128) {
          if ((j+1)<24 && scale_table_works[j+1] == -128) {
            scale_table_works[j+1] = scale_table_works[j];
          }
          if ((j-1)>=0 && scale_table_works[j-1] == -128) {
            scale_table_works[j-1] = scale_table_works[j];
          }
        }
      }
    }
    for (char j=0; j<12; ++j) {
      scale_table[i][j] = scale_table_works[j+6];
    }
  }
}

char volatile scale_table[16][12];
