/*
 * Approach 7.0
 * 
 * With real x/y/z/ graphics
 */

#include <SPI.h>

#include "pwm_dimmer.h"

#define SIZE_X               8
#define SIZE_Y               8
#define SIZE_Z               8
#define SIZE_TLC5940         4

#if (SIZE_X*SIZE_Y) > (SIZE_TLC5940*16)
#error Not enough TCL5940s
#endif

#define GSCLK                6
#define DIAG                 7
#define DIAG_PORT            PORTD
#define DIAG_PIN             PD7
#define XLAT                 8
#define XLAT_PORT            PORTB
#define XLAT_PIN             PB0
#define BLANK                9
#define BLANK_PORT           PORTB
#define BLANK_PIN            PB1
#define VPRG                 10
#define VPRG_PORT            PORTB
#define VPRG_PIN             PB2

#define LAYER_0              2
#define LAYER_0_PORT         PORTD
#define LAYER_0_PIN          PD2
#define LAYER_1              3
#define LAYER_1_PORT         PORTD
#define LAYER_1_PIN          PD3
#define LAYER_2              4
#define LAYER_2_PORT         PORTD
#define LAYER_2_PIN          PD4
#define LAYER_3              5
#define LAYER_3_PORT         PORTD
#define LAYER_3_PIN          PD5
#define LAYER_4              14
#define LAYER_4_PORT         PORTC
#define LAYER_4_PIN          PC0
#define LAYER_5              15
#define LAYER_5_PORT         PORTC
#define LAYER_5_PIN          PC1
#define LAYER_6              16
#define LAYER_6_PORT         PORTC
#define LAYER_6_PIN          PC2
#define LAYER_7              17
#define LAYER_7_PORT         PORTC
#define LAYER_7_PIN          PC3

#define pin_high(port, pin)  port |= _BV(pin)
#define pin_low(port, pin)   port &= ~_BV(pin)

#define PWM_COUNT            4096

volatile uint8_t z = 0;
volatile uint8_t display_mem[SIZE_X][SIZE_Y][SIZE_Z];  
volatile uint8_t tlc5940_buf[24*SIZE_TLC5940];
volatile boolean vsync = false; 

void setup() {

  /* Suspending all interrupts. */
  cli();

  /* Clearing our display memory. */
  memset(display_mem, 0, SIZE_X*SIZE_Y*SIZE_Z);
  
  /* Stopping timer/counter0:1 */
  GTCCR |= _BV(TSM);
  GTCCR |= _BV(PSRASY) | _BV(PSRSYNC);

  /* Output pin configuration.
   * - The GSCLK pin will be driven by timer/counter0   
   * - The XLAT pin will be driven by the timer/counter1 interrupt handler
   * - The BLANK pin will be driven by the timer/counter1 interrupt handler
   * - The LAYER_x pins will be driven by the timer/counter1 interrupt handler
   */
  pin_low(XLAT_PORT, XLAT_PIN);
  pin_high(BLANK_PORT, BLANK_PIN);
  pin_high(VPRG_PORT, VPRG_PIN);
  pin_low(DIAG_PORT, DIAG_PIN);
  pin_high(LAYER_0_PORT, LAYER_0_PIN);
  pin_high(LAYER_1_PORT, LAYER_1_PIN);
  pin_high(LAYER_2_PORT, LAYER_2_PIN);
  pin_high(LAYER_3_PORT, LAYER_3_PIN);
  pin_high(LAYER_4_PORT, LAYER_4_PIN);
  pin_high(LAYER_5_PORT, LAYER_5_PIN);
  pin_high(LAYER_6_PORT, LAYER_6_PIN);
  pin_high(LAYER_7_PORT, LAYER_7_PIN);
  pinMode(GSCLK, OUTPUT);
  pinMode(XLAT, OUTPUT);
  pinMode(BLANK, OUTPUT);
  pinMode(VPRG, OUTPUT);
  pinMode(DIAG, OUTPUT);
  pinMode(LAYER_0, OUTPUT);
  pinMode(LAYER_1, OUTPUT);
  pinMode(LAYER_2, OUTPUT);
  pinMode(LAYER_3, OUTPUT);
  pinMode(LAYER_4, OUTPUT);
  pinMode(LAYER_5, OUTPUT);
  pinMode(LAYER_6, OUTPUT);
  pinMode(LAYER_7, OUTPUT);

  /* Enabling SPI */
  SPI.begin();

  /* Loading the TLC5940 Dot Correction registers with all ones to enable maximum current.
   * With a 2k Iref resistor, this will give us 19,6 mA. */
  memset(tlc5940_buf, 0xFF, 12*SIZE_TLC5940);
  SPI.beginTransaction(SPISettings(30000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(tlc5940_buf, 12*SIZE_TLC5940);
  SPI.endTransaction();
  pin_high(XLAT_PORT, XLAT_PIN);
  pin_low(XLAT_PORT, XLAT_PIN);  
  pin_low(VPRG_PORT, VPRG_PIN);
  
  /* Loading the TLC5940 Grayscale registers with all zeros to switch the LEDs off. */
  memset(tlc5940_buf, 0, 24*SIZE_TLC5940);
  SPI.beginTransaction(SPISettings(30000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(tlc5940_buf, 24*SIZE_TLC5940);
  SPI.endTransaction();
  pin_high(XLAT_PORT, XLAT_PIN);
  pin_low(XLAT_PORT, XLAT_PIN); 

  /* timer/counter0 config:
   * - Clock source is clkIO/1 (no prescaling so we get a 16 MHz input)
   * - Waveform generation mode 7 (fast PWM) with TOP value (in OCR0A) of 3.
   * - Output on pin OC0A (digital 6) in toggle mode, providing a 16 MHz / 2 / (3 + 1) = 2 MHz signal.
   * - No interrupts.
   */
  TCNT0  = 0;
  TCCR0A = _BV(COM0A0) | _BV(WGM01) | _BV(WGM00);
  TCCR0B = _BV(WGM02) | _BV(CS00);
  OCR0A  = 3;
  TIMSK0 = 0;
  
  /* timer/counter1 config:
   * - Clock source is clkIO/8 (with this prescaling we get a 16/8 = 2 MHz input)
   * - Waveform generation mode 0 (normal), we'll clear the counter in the interrupt handler.
   * - No output.
   * - Interrupt on compare with OCR1A; we'll count to PWM_COUNT. Resulting timings:
   *      - one layer will be displayed for 2048 microseconds. We need about 516 microseconds
   *        to do the display work, leaving about 75% of processing capacity for doing fun stuff
   *      - refresh frequency for the entire cube will be 61 Hz, which is still nice.
   */
  TCNT1  = 0;
  TCCR1A = 0;
  TCCR1B = _BV(CS11);
  OCR1A  = PWM_COUNT;
  TIMSK1 = _BV(OCIE1A);

  /* Starting timer/counter0:1 */
  GTCCR &= ~_BV(TSM);
  
  /* Allowing interrupts. */
  sei();
  
}

ISR(TIMER1_COMPA_vect) {
  /* Pulse BLANK. Within the BLANK pulse, switch z, pulse XLAT. We're now showing z 'z' */
  pin_high(BLANK_PORT, BLANK_PIN);
  switch(z) {
    case 0: pin_high(LAYER_7_PORT, LAYER_7_PIN); pin_low(LAYER_0_PORT, LAYER_0_PIN); z++; break;
    case 1: pin_high(LAYER_0_PORT, LAYER_0_PIN); pin_low(LAYER_1_PORT, LAYER_1_PIN); z++; break;
    case 2: pin_high(LAYER_1_PORT, LAYER_1_PIN); pin_low(LAYER_2_PORT, LAYER_2_PIN); z++; break;
    case 3: pin_high(LAYER_2_PORT, LAYER_2_PIN); pin_low(LAYER_3_PORT, LAYER_3_PIN); z++; break;
    case 4: pin_high(LAYER_3_PORT, LAYER_3_PIN); pin_low(LAYER_4_PORT, LAYER_4_PIN); z++; break;
    case 5: pin_high(LAYER_4_PORT, LAYER_4_PIN); pin_low(LAYER_5_PORT, LAYER_5_PIN); z++; break;
    case 6: pin_high(LAYER_5_PORT, LAYER_5_PIN); pin_low(LAYER_6_PORT, LAYER_6_PIN); z++; break;
    case 7: pin_high(LAYER_6_PORT, LAYER_6_PIN); pin_low(LAYER_7_PORT, LAYER_7_PIN); z=0; break;
  }
  pin_high(XLAT_PORT, XLAT_PIN);
  pin_low(XLAT_PORT, XLAT_PIN);
  pin_low(BLANK_PORT, BLANK_PIN);
  
  /* Resetting timer/counter1 */
  TCNT1 = 0;

  pin_high(DIAG_PORT, DIAG_PIN);
  /* Fill new data buffer for layer 'z' - one higher than what we're displaying. On an 8x8 cube, this part takes 385 microseconds. */
  memset(tlc5940_buf, 0, 24*SIZE_TLC5940);
  for(uint8_t x = 0; x < SIZE_X; x++) {
    for(uint8_t y = 0; y < SIZE_Y; y++) {
      uint16_t intensity_pwm = pgm_read_word_near(dimmer_values + display_mem[x][y][z]);
      uint16_t led_index = y * SIZE_X + x;
      uint16_t led_index_half = led_index / 2;
      uint8_t led_index_parity = led_index % 2;
      if(led_index_parity == 0) {
        uint8_t idx_lower8 = SIZE_TLC5940 * 24 - 1 - led_index_half*3;
        uint8_t idx_upper4 = idx_lower8 - 1;
        uint8_t bits_lower8 = intensity_pwm & 0xFF;
        uint8_t bits_upper4 = intensity_pwm >> 8; 
        tlc5940_buf[idx_lower8] = bits_lower8;
        tlc5940_buf[idx_upper4] = (tlc5940_buf[idx_upper4] & 0xF0) | bits_upper4;
      } else {
        uint8_t idx_lower4 = SIZE_TLC5940 * 24 - 1 - led_index_half*3 - 1;
        uint8_t idx_upper8 = idx_lower4 - 1;
        uint8_t bits_lower4 = (intensity_pwm & 0x0F) << 4;
        uint8_t bits_upper8 = intensity_pwm >> 4;
        tlc5940_buf[idx_lower4] = (tlc5940_buf[idx_lower4] & 0x0F) | bits_lower4;
        tlc5940_buf[idx_upper8] = bits_upper8;
      }
    }
  }

  /* Send the data to the TLCs. On an 8x8 cube with 4 TLCs, this takes 130 microseconds. */
  SPI.beginTransaction(SPISettings(30000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(tlc5940_buf, 24*SIZE_TLC5940);
  SPI.endTransaction();
  pin_low(DIAG_PORT, DIAG_PIN);

  if(z == 7) vsync = true;

}

uint8_t counter = 0;
uint16_t counter_top = 20;

void loop() {
  while(!vsync);
  vsync = false;
  if(counter++ >= counter_top) {
    counter = 0;
    nextimage();
  }
  counter_top = 5 + analogRead(7)/8;
}

uint8_t pos0_x = 0, pos0_z = 0, pos1_x = 1, pos1_z = 0, pos2_x = 2, pos2_z = 0;

void nextimage() {
  boolean done = false;
  while(!done) {
    uint8_t candidate_x = pos0_x;
    uint8_t candidate_z = pos0_z;
    done = true;
    switch(random(4)) {
      case 0: candidate_x--; break;
      case 1: candidate_x++; break;
      case 2: candidate_z--; break;
      case 3: candidate_z++; break;
    }
    if(candidate_x > 2 || candidate_z > 2) done = false;
    if(candidate_x == pos1_x && candidate_z == pos1_z) done = false;
    if(done) {
      pos2_x = pos1_x;
      pos2_z = pos1_z;
      pos1_x = pos0_x;
      pos1_z = pos0_z;
      pos0_x = candidate_x;
      pos0_z = candidate_z;
    }
  }
    
  memset(display_mem, 0, SIZE_X*SIZE_Y*SIZE_Z);
  display_mem[pos0_x][0][pos0_z] = 255;
  display_mem[pos1_x][0][pos1_z] = 127;
  display_mem[pos2_x][0][pos2_z] = 63;

}
