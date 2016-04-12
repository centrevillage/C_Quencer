#include "dac.h"

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

// ldac_pin = PB1
// ss_pin = PB0
void output_osc() {
  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;
  //trans_spi((i >> 8) | 0x30);
  //trans_spi(i & 0xff);
  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
}