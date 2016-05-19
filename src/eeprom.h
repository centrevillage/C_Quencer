#include <avr/io.h>
#include <avr/eeprom.h> 

#ifndef CQ_EEPROM_H_
#define CQ_EEPROM_H_

typedef struct {

  struct ScalePreset {
    uint16_t record_flag; 
    uint16_t scales[16];
  } scale_preset;

  struct PatternPreset {
    uint16_t record_flag; 
    uint8_t patterns[16][16];
  } pattern_preset;

} PresetInfo;

extern volatile PresetInfo preset_info;

void write_scale_preset(uint8_t preset_num, uint16_t scale_value);
void reset_scale_preset(uint8_t preset_num);
void write_pattern_preset(uint8_t preset_num, uint8_t values[16]);
void reset_pattern_preset(uint8_t preset_num);
void load_preset();
void try_reset_eeprom();

#endif /* CQ_EEPROM_H_ */
