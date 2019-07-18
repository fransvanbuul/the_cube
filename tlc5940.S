/*
 * Assembly routines for mapping the 8-bit intensity values in display_mem to 12-bit PWM duty cycles
 * and sending the data to the array of TLC5940s via hardware SPI.
 * Doing this in assembly allows us to optimize for speed by smart use of registers, addressing modes
 * and by continuing calculations while a byte is being transferred. 
 *
 * Just as easy reference during coding, the rules for register usage from C:
 * 
 * r0                                      temporary       Save and restore if using
 * r1                                      zero            Must clear before returning
 * r2-r17, r28, r29 (=Y)                   “call-saved”    Save and restore if using
 * r18-r25, r26, r27 (=X), r30, r31 (=Z)   “call-used”     Can freely use
 */

#include <avr/io.h>
#include "cube_sizes.h"

    .extern     z
    .extern     display_mem
    .extern     dimmer_values

    .macro      spi_send_byte_nowait b:req
    OUT         _SFR_IO_ADDR(SPDR), \b    
    .endm

    .macro      spi_send_waitcomplete b:req
    NOP                  // leads to measureable performance improvement, don't know exactly why
spi_send_waitcomplete_\@:
    IN          \b,  _SFR_IO_ADDR(SPSR)
    SBRS        \b,  SPIF
    RJMP        spi_send_waitcomplete_\@   
    .endm
    
    .section    .text

dimmer_values:
    .int        0, 4, 6, 7, 9, 11, 12, 14, 15, 17,\
    19, 20, 22, 24, 26, 27, 29, 31, 33, 35,\
    37, 39, 41, 43, 45, 47, 49, 51, 53, 55,\
    58, 60, 62, 65, 67, 69, 72, 74, 77, 79,\
    82, 85, 87, 90, 93, 95, 98, 101, 104, 107,\
    110, 113, 116, 119, 122, 126, 129, 132, 136, 139,\
    143, 146, 150, 153, 157, 161, 165, 168, 172, 176,\
    180, 184, 189, 193, 197, 201, 206, 210, 215, 219,\
    224, 229, 234, 238, 243, 248, 253, 259, 264, 269,\
    275, 280, 286, 291, 297, 303, 309, 315, 321, 327,\
    333, 340, 346, 353, 359, 366, 373, 380, 387, 394,\
    401, 408, 416, 424, 431, 439, 447, 455, 463, 471,\
    480, 488, 497, 506, 515, 524, 533, 542, 552, 561,\
    571, 581, 591, 601, 611, 622, 632, 643, 654, 665,\
    676, 688, 699, 711, 723, 735, 748, 760, 773, 786,\
    799, 812, 825, 839, 853, 867, 881, 896, 910, 925,\
    940, 956, 971, 987, 1003, 1019, 1036, 1053, 1070, 1087,\
    1104, 1122, 1140, 1158, 1177, 1196, 1215, 1234, 1254, 1274,\
    1294, 1315, 1336, 1357, 1379, 1400, 1423, 1445, 1468, 1491,\
    1515, 1538, 1563, 1587, 1612, 1637, 1663, 1689, 1716, 1742,\
    1770, 1797, 1825, 1854, 1883, 1912, 1942, 1972, 2002, 2033,\
    2065, 2097, 2129, 2162, 2196, 2230, 2264, 2299, 2335, 2371,\
    2407, 2444, 2482, 2520, 2559, 2598, 2638, 2679, 2720, 2761,\
    2804, 2847, 2890, 2934, 2979, 3025, 3071, 3118, 3165, 3214,\
    3263, 3313, 3363, 3414, 3466, 3519, 3573, 3627, 3682, 3738,\
    3795, 3852, 3911, 3970, 4030, 4095;
  
/*    
 *     Send a single byte over SPI.
 *     In:    R24 (first 8-bit argument): the byte 
 *     Out:   void
 */
    .global     spi_send_byte
spi_send_byte:
    spi_send_byte_nowait R24
    spi_send_waitcomplete R24
    RET 
    
/*
 *     Send data from display_mem to the TLC array.
 *     
 *     History of performance optimizations:
 *     (all for 8x8x8 cube using 4 TLCs, in microseconds):
 *     
 *                                            buffer fill    sending    total
 * Original code (using PROGMEM):              385             135       520
 * Using SRAM instead of PROGMEM:              350             135       485
 * Without the upfront memset:                 320             135       355
 * Direct pointer arithmetic in C:             134             135       269
 * Bufferless-sending in C:                                              211
 * Assembly routine wait calculate-while-wait (dimmersval in SRAM)       140
 * Assembly routine wait calculate-while-wait (dimmersval in progmem)    148
 * 
 * Theoretical minimum, taking into account 8 MHz SPI speed:              96     
 *     
 *
 *     Registers used by routine:
 *       R0, R1, R18, R19, R22  -> local work registers, no particular function. R1:R0 contains MUL results
 *       R20, R21 -> fixed to beginning of dimmer_values array
 *       R27:R26 (X) -> display_mem_ptr
 *       R25 -> skip_led_count
 *       R24 -> tlc_triple_count
 *       R23 -> always 0
 *       R2 -> always 4
 */
    .global     send_data_to_tlc
send_data_to_tlc:
// save registers
    PUSH        R0
    PUSH        R2
    
// Initializing X register: equivalent of uint8_t* display_mem_ptr = &(display_mem[z+1][0][0])
    LDS         R18, z
    INC         R18
    LDI         R19, (SIZE_X*SIZE_Y)
    MUL         R18, R19                // now R1:R0 = (z+1)*(SIZE_X*SIZE_Y)  (which can range from 1..512)
    LDI         R18, lo8(display_mem)
    LDI         R19, hi8(display_mem)
    ADD         R0,  R18
    ADC         R1,  R19                // R1:R0  +=   &(display_mem[0][0][0])
    MOVW        X,   R0                 // XH:XL  :=   R1:R0

// Initializing R25 register: equivalent of uint8_t  skip_led_count = 16*SIZE_TLC5940 - SIZE_X*SIZE_Y
    LDI         R25, (16*SIZE_TLC5940 - SIZE_X*SIZE_Y)

// Initializing R24 register: equivalent of uint8_t  tlc_triple_count = 8*SIZE_TLC5940
    LDI         R24, (8*SIZE_TLC5940) 

// Initializing R23 as zero-register since R1 (GCC zero-reg) will get trashed during MUL instructions
    LDI         R23, 0 

// Initializing R21:R20 to &(dimmer_valuess[0]) (in progmem)
    LDI         R20,  lo8(dimmer_values)    
    LDI         R21,  hi8(dimmer_values)

// Initializing R2 as constant 4
    LDI         R18,  4
    MOV         R2,   R18    
        
// Start sending triples of 3 bytes to TLC5940 array
send_triples_loop:
    TST         R25                    // if skip_led_count
    BRNE        one_or_two_leds        //       !=, go to one_or_two_leds

two_leds:
    LD          R18, -X                // R18 := *(--display_mem_ptr), intensity of LED
    MOVW        Z,   R20               // ZH:ZL = R21:R20
    MUL         R18, R2                // R1:R0 = 4*R18
    ADD         ZL,  R0
    ADC         ZH,  R1                // now, ZH:ZL = &(dimmer_values[intensity])
    LPM         R18, Z+                
    LPM         R19, Z                 // now, R19:R18 has the PWM duty cycle, but we need to shift left by 4 bits

    SWAP        R18
    MOV         R22, R18               
    SWAP        R19
    ANDI        R18, 0x0F
    OR          R19, R18
    ANDI        R22, 0xF0

    spi_send_waitcomplete R0          // given the amoutn of prior instructions and our SPI speed, we don't have to wait here anymore 
    spi_send_byte_nowait R19          // sending the high 8 bits of the first LED 

    LD          R18, -X                // R18 := *(--display_mem_ptr), intensity of LED
    MOVW        Z,   R20               // ZH:ZL = R21:R20
    MUL         R18, R2                // R1:R0 = 4*R18
    ADD         ZL,  R0
    ADC         ZH,  R1                // now, ZH:ZL = &(dimmer_values[intensity])
    LPM         R18, Z+                
    LPM         R19, Z                 // now, R19:R18 has the PWM duty cycle

    OR          R19, R22
    spi_send_waitcomplete R0   
    spi_send_byte_nowait R19          
    spi_send_waitcomplete R0   
    spi_send_byte_nowait R18          
    
    DEC         R24                    // tlc_triple_count--
    BRNE        send_triples_loop      // loop while tlc_triple_count != 0
    RJMP        end_send_triples_loop

one_or_two_leds:
    
    spi_send_waitcomplete R0   
    spi_send_byte_nowait R23           // we are in a 0 or 1 led triple; start by sending zero byte

    DEC         R25                    // skip_led_count--
    BREQ        one_led                // if skip_led_count == 0, go to one_led

zero_leds:
                                         
    spi_send_waitcomplete R0   
    spi_send_byte_nowait R23           // this is a 0 led triple; send two more zero byte
    spi_send_waitcomplete R0
    spi_send_byte_nowait R23           // send zero byte
    
    DEC         R25                    // skip_led_count--

    DEC         R24                    // tlc_triple_count--
    BREQ        end_send_triples_loop      // loop while tlc_triple_count != 0
    RJMP        send_triples_loop
    
one_led:

    LD          R18, -X                // R18 := *(--display_mem_ptr), intensity of LED
    MOVW        Z,   R20               // ZH:ZL = R21:R20
    MUL         R18, R2                // R1:R0 = 4*R18
    ADD         ZL,  R0
    ADC         ZH,  R1                // now, ZH:ZL = &(dimmer_values[intensity])  
    LPM         R18, Z+                
    LPM         R19, Z                 // now, R19:R18 has the PWM duty cycle

    spi_send_waitcomplete R0   
    spi_send_byte_nowait R19          
    spi_send_waitcomplete R0      
    spi_send_byte_nowait R18          

    DEC         R24                    // tlc_triple_count--
    BREQ        end_send_triples_loop      // loop while tlc_triple_count != 0
    RJMP        send_triples_loop

end_send_triples_loop:
    
//  restore registers
    POP         R2
    CLR         R1
    POP         R0
    RET   

    .end
