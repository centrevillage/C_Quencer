#ifndef __CQ_BIT_MAGIC_H
#define __CQ_BIT_MAGIC_H

inline uint8_t bit_count_in_8bit(uint8_t bits) {
  bits = bits - ((bits >> 1) & 0b01010101);
  bits = (bits & 0b00110011) + ((bits >> 2) & 0b00110011);
  bits = (bits & 0b00001111) + (bits >> 4);
  return bits;
}

inline uint16_t bit_count_in_16bit(uint16_t bits) {
  bits = bits - ((bits >> 1) & 0b0101010101010101);
  bits = (bits & 0b0011001100110011) + ((bits >> 2) & 0b0011001100110011);
  bits = (bits + (bits >> 4)) & 0b0000111100001111;
  bits = bits + (bits >> 8);
  return bits & 0x1F;
}

inline uint8_t bit_index_in_8bit(uint8_t bits) {
  return bit_count_in_8bit((bits & (-bits)) - 1);
}

inline uint16_t bit_index_in_16bit(uint16_t bits) {
  return bit_count_in_16bit((bits & (-bits)) - 1);
}

inline uint16_t bit_left_rotate_in_16bit(uint16_t bits, uint8_t rotate /* 0 to 15 */) {
  return (bits << rotate) | (bits >> (16 - rotate));
}

inline uint16_t bit_left_rotate_in_16bit_with_length(uint16_t bits, uint8_t rotate /* 0 to length */, uint8_t length /* 1.. 16 */) {
  if (length == 16) {
    return bit_left_rotate_in_16bit(bits, rotate);
  }
  return ((bits << rotate) | (bits >> (length - rotate))) & ((1<<(length)) - 1);
}

inline uint16_t bit_right_rotate_in_16bit(uint16_t bits, uint8_t rotate /* 0 to 15 */) {
  return (bits >> rotate) | (bits << (16 - rotate));
}


#define extract_most_right1(bits) ((bits) & -(bits))
#define reset_most_right1(bits) ((bits) & ((bits)-1))

#endif /* __CQ_BIT_MAGIC_H */
