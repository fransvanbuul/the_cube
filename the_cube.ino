/*
 * LED cube
 * 
 * Basic set-up of the electronics
 * - 8 x 8 x 8 cube, layers have common anode, columns have common cathode 
 * - layers sourced from P-channel MOSFETs (IRF9520), driven directly from Arduino D2..D5 (2..5) and A0..A3 (14..17)
 * - one layer is active at a time (multiplexing), 2 ms per layer, 61 Hz refresh frequency for the 8-layer cube
 * - led intensity controlled through 4 TLC5940s, controlled through Arduino's hardware SPI
 * - orientation information from a MPU-6050 on a GY-521 breakout board controlled through Arduino's hardware I2C
 * 
 */

#include <SPI.h>
#include <MPU6050_tockn.h>
#include <Wire.h>

#include "cube_shared.h"
#include "pwm_dimmer.h"
#include "cube_wiring.h"

uint8_t display_mem[SIZE_X][SIZE_Y][SIZE_Z];  

static MPU6050 mpu6050(Wire);
static uint8_t tlc5940_buf[24*SIZE_TLC5940];

void setup() {

  /* Suspending all interrupts. */
  cli();
  
  /* Stopping all timers. */
  GTCCR |= _BV(TSM);
  GTCCR |= _BV(PSRASY) | _BV(PSRSYNC);
  
  /* Clearing our display memory. */
  memset(display_mem, 0, SIZE_X*SIZE_Y*SIZE_Z);

  /* Output pin configuration.
   * - The GSCLK pin will be driven by timer/counter2
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

  /* Enabling SPI for the TLC5940 */
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

  /* timer/counter1 config (powering XLAT/BLANK)
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

  /* timer/counter2 config (powering GSCLK on the TLC5940)
   * - Clock source is clkIO/1 (no prescaling so we get a 16 MHz input)
   * - Waveform generation mode 2 (Clear Timer on Compare Match / CTC) with TOP value (in OCR2A) of 3.
   * - Output on pin OC2B (digital 3) in toggle mode, providing a 16 MHz / 2 (toggle) / (3 + 1) = 2 MHz signal.
   * - No interrupts.
   */
  TCNT2  = 0;
  TCCR2A = _BV(COM2B0) | _BV(WGM21);
  TCCR2B = _BV(CS20);
  OCR2A  = 3;
  TIMSK2 = 0;
  
  /* Starting timer/counters */
  GTCCR &= ~_BV(TSM);
  
  /* Allowing interrupts. */
  sei();

  /* Enabling serial comms for connection with PC */
  Serial.begin(2000000);

  /* Enabling I2C for the MPU-6050 */
  Wire.begin();

  /* Initialize the MPU-6050 */
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  Serial.println();
}

ISR(TIMER1_COMPA_vect) {
  static uint8_t z = 0;
  
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
}

void loop() {
  snake3d_loop();
}
