#include <avr/io.h>
#include <string.h>
#include <util/delay.h>

#define LDAC_MASK 0b00000010
#define SS_MASK   0b00000001
#define LDAC_SS_MASK = 0b00000011

static unsigned int  bpmX10 = 1200;
static unsigned char devide_count = 1;
static unsigned char current_pos = 0;
static unsigned char active_seq[16];

union ControllerState {
  uint8_t data;

  struct {
    uint8_t rec :1;   // on recording
    uint8_t start :1; // on sequence running
    uint8_t sub :1;   // on sub function
    uint8_t hid :1;   // on hidden function
    uint8_t func_lock :1; // on sub function lock
  } bit;

};

union ControllerState current_state;

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

void adc_init() {
 	// AVCC as ref voltage
	ADMUX =  (1<<REFS0);
	ADCSRA = (1<<ADPS2) |(1<<ADPS1)| (1<<ADPS0);     // adc prescaler (must be between 50 and 200kHz)
	ADCSRA |= (1<<ADEN);                  // ADC enable
 	// dummy readout
	ADCSRA |= (1<<ADSC);                  // single readout
	while (ADCSRA & (1<<ADSC) ) {}        // wait to finish
}

uint16_t adc_read(uint8_t pin) {
  ADMUX |= pin;
	ADCSRA |= (1<<ADSC);            // single readout
	while (ADCSRA & (1<<ADSC) ) {}  // wait to finish
	return ADCW;
};

// seq task
static int g_step_i = 0;
void step_seq() {
  active_seq[g_step_i] = 1;
  if (++g_step_i >= 16) {
    g_step_i = 0;
    memset(active_seq, 0, sizeof(active_seq));
  }
}

// TODO: アナログ入力についてはCPUの時間を空けるために順番に値を読む
// input task
static float g_fill_len_rate = 0.0f;
void handle_input() {

  uint16_t fill_len_v = adc_read(0);
  uint16_t rot_len_v  = adc_read(1);
  uint16_t seq_pat_v  = adc_read(2);
  uint16_t range_v    = adc_read(3);
  unsigned char func_b  = PIND & _BV(5);
  //unsigned char start_b = PIND & _BV(6); // ピン割り込み？
  //unsigned char tap_b   = PIND & _BV(7); // ピン割り込み？
  unsigned char rec_b   = PINC & _BV(5);

  g_fill_len_rate = (float) fill_len_v / 1024.0f;
}

// led_task
// charlieplexing
// use PD0 - PD5
int g_led_i = 0;
const uint8_t LED_MASK = 0b00011111;
void output_led() {
  DDRD = 0;
  PORTD &= (uint8_t)~LED_MASK;
  if (active_seq[g_led_i]) {
    switch (g_led_i) {
      case 0:
        PORTD |= 0b00000010;
        DDRD  = 0b00000011;
        break;
      case 1:
        PORTD |= 0b00000001;
        DDRD  = 0b00000011;
        break;
      case 2:
        PORTD |= 0b00000100;
        DDRD  = 0b00000101;
        break;
      case 3:
        PORTD |= 0b00000001;
        DDRD  = 0b00000101;
        break;
      case 4:
        PORTD |= 0b00000100;
        DDRD  = 0b00000110;
        break;
      case 5:
        PORTD |= 0b00000010;
        DDRD  = 0b00000110;
        break;
      case 6:
        PORTD |= 0b00001000;
        DDRD  = 0b00001010;
        break;
      case 7:
        PORTD |= 0b00000010;
        DDRD  = 0b00001010;
        break;
      case 8:
        PORTD |= 0b00001000;
        DDRD  = 0b00001100;
        break;
      case 9:
        PORTD |= 0b00000100;
        DDRD  = 0b00001100;
        break;
      case 10:
        PORTD |= 0b00010000;
        DDRD  = 0b00010100;
        break;
      case 11:
        PORTD |= 0b00000100;
        DDRD  = 0b00010100;
        break;
      case 12:
        PORTD |= 0b00010000;
        DDRD  = 0b00011000;
        break;
      case 13:
        PORTD |= 0b00001000;
        DDRD  = 0b00011000;
        break;
      case 14:
        PORTD |= 0b00010000;
        DDRD  = 0b00010001;
        break;
      case 15:
        PORTD |= 0b00000001;
        DDRD  = 0b00010001;
        break;
      default:
        break;
    }
  }
  if (++g_led_i >= 16) {
    g_led_i = 0;
  }
}

// DAC task
// ldac_pin = PB1
// ss_pin = PB0
void output_osc() {
  //// test
  //int max_i = 4096 * g_fill_len_rate;
  //for (int i = 0; i < max_i; i += 4) {
  //  //digitalWrite(ldac_pin, HIGH);
  //  //digitalWrite(ss_pin, LOW);
  //  PORTB = (PORTB & ~LDAC_SS_MASK) | LDAC_MASK;

  //  SPI.transfer((i >> 8) | 0x30);
  //  SPI.transfer(i & 0xff);

  //  //digitalWrite(ss_pin, HIGH);
  //  //digitalWrite(ldac_pin, LOW);
  //  PORTB = (PORTB & ~LDAC_SS_MASK) | SS_MASK;
  //}
}

void setup_pins() {
  // for Arduino
  //for (int i = 0; i < 5; ++i) {
  //  pinMode(i, OUTPUT);
  //  digitalWrite(i, LOW);
  //}
  //pinMode(switch1_pin, INPUT_PULLUP);
  //pinMode(switch2_pin, INPUT_PULLUP);
  //pinMode(switch3_pin, INPUT_PULLUP);
  //pinMode(switch4_pin, INPUT_PULLUP);
  //pinMode(ss_pin, OUTPUT);
  //pinMode(ldac_pin, OUTPUT);

  // 0-4: LED, 5-7: switch in
  DDRD = 0;
  // switch with pullup
  PORTD = 0b11100000;
  // OUTPUT: CS & LDAC & GATE & MOSI & SCK, INPUT: MISO
  DDRB = _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(5);
  // INPUT: POT1-4 & TRIG & REC SW & RESET
  DDRC = 0;
  // switch with pullup
  PORTC = _BV(5);
}

void timer_init() {
}

// the setup function runs once when you press reset or power the board
void setup() {
  ACSR |= (1<<ACD); //analog comparator off

  // initialize pin
  setup_pins();

  // initialize sequence
  memset(active_seq, 0, sizeof(active_seq));

  adc_init();
  spi_init();
}

unsigned long g_current_usec = 0;
void loop() {
  //step_seq(); // process Timer2?
  output_led();
  handle_input();
  //output_osc(); // process Timer1?
}

// from Arduino ==
int main(void) {
  timer_init();
	setup();
	for (;;) { loop(); }
	return 0;
}
