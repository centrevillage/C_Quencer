#include <avr/pgmspace.h>
#include <string.h>
#include "eeprom.h"
#include "pattern.h"
#include "scale.h"

volatile PresetInfo preset_info;

#define SCALE_PRESET_SIZE 34

void write_scale_preset(uint8_t preset_num, uint16_t scale_value){
  unsigned int addr = 0;
  preset_info.scale_preset.record_flag |= (1<<preset_num);
  eeprom_busy_wait();
  eeprom_write_word((uint16_t*)addr, preset_info.scale_preset.record_flag);
  addr+=2;

  preset_info.scale_preset.scales[preset_num] = scale_value;
  eeprom_busy_wait();
  addr+=(preset_num*2);
  eeprom_write_word((uint16_t*)addr, preset_info.scale_preset.scales[preset_num]);

  update_scale_table(preset_info.scale_preset.scales);
}
void reset_scale_preset(uint8_t preset_num){
  preset_info.scale_preset.record_flag &= ~(1<<preset_num);
  preset_info.scale_preset.scales[preset_num] = pgm_read_word(&(scale_defs[preset_num]));
  eeprom_busy_wait();
  eeprom_write_word((uint16_t*)0, preset_info.scale_preset.record_flag);

  update_scale_table(preset_info.scale_preset.scales);
}
void write_pattern_preset(uint8_t preset_num, uint8_t values[16]){
  unsigned int addr = SCALE_PRESET_SIZE;
  preset_info.pattern_preset.record_flag |= (1<<preset_num);
  eeprom_busy_wait();
  eeprom_write_word((uint16_t*)addr, preset_info.pattern_preset.record_flag);
  addr+=2;

  addr+=(preset_num * 16);
  for (int i=0; i<16; ++i) {
    preset_info.pattern_preset.patterns[preset_num][i] = values[i];
    eeprom_busy_wait();
    eeprom_write_byte((uint8_t*)addr, preset_info.pattern_preset.patterns[preset_num][i]);
    ++addr;
  }
}
void reset_pattern_preset(uint8_t preset_num){
  preset_info.pattern_preset.record_flag &= ~(1<<preset_num);
  for (int j=0; j<16; ++j) {
    preset_info.pattern_preset.patterns[preset_num][j] = pgm_read_byte(&(scale_patterns[preset_num][j]));
  }
  eeprom_busy_wait();
  eeprom_write_word((uint16_t*)SCALE_PRESET_SIZE, preset_info.pattern_preset.record_flag);
}

void try_reset_eeprom() {
  const char* symbol = "C_Quencer 0.4";
  char id_str[13];
  for (int i=0; i<13; ++i) {
    eeprom_busy_wait();
    id_str[i] = eeprom_read_byte((uint8_t*)(i+1000));
  }
  if (strncmp(id_str, symbol, sizeof(id_str)) != 0) {
    for (int i=0; i<1000; ++i) {
      eeprom_busy_wait();
      eeprom_write_byte((uint8_t*)i, 0);
    }
    eeprom_write_block(symbol, (uint8_t*)1000, sizeof(symbol));
    for (int i=0; i<13; ++i) {
      eeprom_busy_wait();
      eeprom_write_byte((uint8_t*)(i+1000), symbol[i]);
    }
  }
}

void load_preset(){
  try_reset_eeprom();

  unsigned int addr;

  addr = 0;

  eeprom_busy_wait();
  preset_info.scale_preset.record_flag = eeprom_read_word((uint16_t *)addr);
  addr += 2;

  for (int i=0; i<16; ++i) {
    eeprom_busy_wait();
    preset_info.scale_preset.scales[i] = eeprom_read_word((uint16_t *)addr);
    addr += 2;
  }

  eeprom_busy_wait();
  preset_info.pattern_preset.record_flag = eeprom_read_word((uint16_t *)addr);
  addr += 2;

  for (int i=0; i<16; ++i) {
    for (int j=0; j<16; ++j) {
      eeprom_busy_wait();
      preset_info.pattern_preset.patterns[i][j] = eeprom_read_byte((uint8_t *)addr);
      ++addr;
    }
  }

  for (int i=0; i<16; ++i) {
    if (!(preset_info.scale_preset.record_flag & (1<<i))) {
      preset_info.scale_preset.scales[i] = pgm_read_word(&(scale_defs[i]));
    }
    if (!(preset_info.pattern_preset.record_flag & (1<<i))) {
      for (int j=0; j<16; ++j) {
        preset_info.pattern_preset.patterns[i][j] = pgm_read_byte(&(scale_patterns[i][j]));
      }
    }
  }

  update_scale_table(preset_info.scale_preset.scales);
}
