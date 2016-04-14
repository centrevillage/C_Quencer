#include "dac.h"
#include "input.h"

// C1 - B8
static uint16_t pitch_to_tableidx_x100[96] = {
52, 55, 59, 62, 66, 70, 74, 78, 83, 88, 93, 99, 105, 111, 117, 124, 132, 140, 148, 157, 166, 176, 186, 198, 209, 222, 235, 249, 264, 279, 296, 314, 332, 352, 373, 395, 419, 443, 470, 498, 527, 559, 592, 627, 664, 704, 746, 790, 836, 886, 940, 996, 1055, 1118, 1184, 1254, 1329, 1408, 1492, 1580, 1673, 1773, 1879, 1991, 2110, 2235, 2368, 2509, 2658, 2816, 2983, 3161, 3349, 3547, 3759, 3982, 4219, 4470, 4736, 5018, 5316, 5632, 5967, 6322, 6698, 7095, 7518, 7965, 8438, 8940, 9472, 10035, 10632, 11264, 11934, 12643
};

void spi_init() {
  //Enable SPI, Master, set clock rate fck/16
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void trans_spi(char data) {
   //Start transmission
   SPDR = data;
   // Wait for transmission complete
   while(!(SPSR & (1<<SPIF)));
}

void output_dac_a(uint16_t data) {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  trans_spi((data >> 8) | _BV(4) | _BV(5)); // OUT A, gain x1, no shutdown
  trans_spi(data & 0xFF);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}

void output_dac_b(uint16_t data) {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  trans_spi((data >> 8) | _BV(7) | _BV(4) | _BV(5)); // OUT B, gain x1, no shutdown
  trans_spi(data & 0xFF);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}

char read_spi() {
   char data;
   //Output Clolk without data
   SPDR = 0;
   // Wait for transmission complete
   while(!(SPSR & (1<<SPIF)));
   //Recieve 1 byte data
   data = SPDR;
   return data;
}

//C0- B7 = 0 - 96
//A3(45) = 440
//1 timer count = 16kHz = 1/ 16000 s = 62.5us / 1 cycle
static uint8_t current_wave_cycle() {
  //TODO:
}

// ldac_pin = PB1
// ss_pin = PB0
void output_osc(uint16_t timer_count) {
  
  //TODO:
  //output_dac_a()
}
