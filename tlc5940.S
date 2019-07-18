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
 * Assembly routine wait calculate-while-wait (+shifted copy in progmem) 137
 * Assembly routine wait calculate-while-wait (+conditional assembly)    133 
 * 
 * Theoretical minimum, taking into account 8 MHz SPI speed:              96     
 *     
 *
 *     Registers used by routine:
 *       R0, R1, R18, R19, R22  -> local work registers, no particular function. R1:R0 contains MUL results
 *       R20, R21 -> fixed to beginning of dimmer_values array
 *       R16, R17 -> fixed to beginning of dimmer_values_shifted array
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
    PUSH        R16
    PUSH        R17
    
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
#if (SIZE_X*SIZE_Y) < (SIZE_TLC5940*16)
    LDI         R25, (16*SIZE_TLC5940 - SIZE_X*SIZE_Y)
#endif

// Initializing R24 register: equivalent of uint8_t  tlc_triple_count = 8*SIZE_TLC5940
    LDI         R24, (8*SIZE_TLC5940) 

// Initializing R23 as zero-register since R1 (GCC zero-reg) will get trashed during MUL instructions
    LDI         R23, 0 

// Initializing R21:R20 to &(dimmer_valuess[0]) (in progmem)
    LDI         R20,  lo8(dimmer_values)    
    LDI         R21,  hi8(dimmer_values)

// Initializing R17:R16 to &(dimmer_valuess_shifted[0]) (in progmem)
    LDI         R16,  lo8(dimmer_values_shifted)    
    LDI         R17,  hi8(dimmer_values_shifted)    

// Initializing R2 as constant 4
    LDI         R18,  4
    MOV         R2,   R18    
        
// Start sending triples of 3 bytes to TLC5940 array
send_triples_loop:

#if (SIZE_X*SIZE_Y) < (SIZE_TLC5940*16)
    TST         R25                    // if skip_led_count
    BRNE        one_or_two_leds        //       !=, go to one_or_two_leds
#endif

two_leds:
    LD          R18, -X                // R18 := *(--display_mem_ptr), intensity of LED
    MOVW        Z,   R16               // ZH:ZL = R17:R16
    MUL         R18, R2                // R1:R0 = 4*R18
    ADD         ZL,  R0
    ADC         ZH,  R1                // now, ZH:ZL = &(dimmer_values_shifted[intensity])
    LPM         R22, Z+                
    LPM         R19, Z                 // now, R19:R22 has the PWM duty cycle shifted by 4 bits

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

#if (SIZE_X*SIZE_Y) < (SIZE_TLC5940*16)
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
#endif

end_send_triples_loop:
    
//  restore registers
    POP         R17
    POP         R16
    POP         R2
    CLR         R1
    POP         R0
    RET   

dimmer_values:
    .int        0x0000, 0x0004, 0x0006, 0x0007, 0x0009, 0x000b, 0x000c, 0x000e
    .int        0x000f, 0x0011, 0x0013, 0x0014, 0x0016, 0x0018, 0x001a, 0x001b
    .int        0x001d, 0x001f, 0x0021, 0x0023, 0x0025, 0x0027, 0x0029, 0x002b
    .int        0x002d, 0x002f, 0x0031, 0x0033, 0x0035, 0x0037, 0x003a, 0x003c
    .int        0x003e, 0x0041, 0x0043, 0x0045, 0x0048, 0x004a, 0x004d, 0x004f
    .int        0x0052, 0x0055, 0x0057, 0x005a, 0x005d, 0x005f, 0x0062, 0x0065
    .int        0x0068, 0x006b, 0x006e, 0x0071, 0x0074, 0x0077, 0x007a, 0x007e
    .int        0x0081, 0x0084, 0x0088, 0x008b, 0x008f, 0x0092, 0x0096, 0x0099
    .int        0x009d, 0x00a1, 0x00a5, 0x00a8, 0x00ac, 0x00b0, 0x00b4, 0x00b8
    .int        0x00bd, 0x00c1, 0x00c5, 0x00c9, 0x00ce, 0x00d2, 0x00d7, 0x00db
    .int        0x00e0, 0x00e5, 0x00ea, 0x00ee, 0x00f3, 0x00f8, 0x00fd, 0x0103
    .int        0x0108, 0x010d, 0x0113, 0x0118, 0x011e, 0x0123, 0x0129, 0x012f
    .int        0x0135, 0x013b, 0x0141, 0x0147, 0x014d, 0x0154, 0x015a, 0x0161
    .int        0x0167, 0x016e, 0x0175, 0x017c, 0x0183, 0x018a, 0x0191, 0x0198
    .int        0x01a0, 0x01a8, 0x01af, 0x01b7, 0x01bf, 0x01c7, 0x01cf, 0x01d7
    .int        0x01e0, 0x01e8, 0x01f1, 0x01fa, 0x0203, 0x020c, 0x0215, 0x021e
    .int        0x0228, 0x0231, 0x023b, 0x0245, 0x024f, 0x0259, 0x0263, 0x026e
    .int        0x0278, 0x0283, 0x028e, 0x0299, 0x02a4, 0x02b0, 0x02bb, 0x02c7
    .int        0x02d3, 0x02df, 0x02ec, 0x02f8, 0x0305, 0x0312, 0x031f, 0x032c
    .int        0x0339, 0x0347, 0x0355, 0x0363, 0x0371, 0x0380, 0x038e, 0x039d
    .int        0x03ac, 0x03bc, 0x03cb, 0x03db, 0x03eb, 0x03fb, 0x040c, 0x041d
    .int        0x042e, 0x043f, 0x0450, 0x0462, 0x0474, 0x0486, 0x0499, 0x04ac
    .int        0x04bf, 0x04d2, 0x04e6, 0x04fa, 0x050e, 0x0523, 0x0538, 0x054d
    .int        0x0563, 0x0578, 0x058f, 0x05a5, 0x05bc, 0x05d3, 0x05eb, 0x0602
    .int        0x061b, 0x0633, 0x064c, 0x0665, 0x067f, 0x0699, 0x06b4, 0x06ce
    .int        0x06ea, 0x0705, 0x0721, 0x073e, 0x075b, 0x0778, 0x0796, 0x07b4
    .int        0x07d2, 0x07f1, 0x0811, 0x0831, 0x0851, 0x0872, 0x0894, 0x08b6
    .int        0x08d8, 0x08fb, 0x091f, 0x0943, 0x0967, 0x098c, 0x09b2, 0x09d8
    .int        0x09ff, 0x0a26, 0x0a4e, 0x0a77, 0x0aa0, 0x0ac9, 0x0af4, 0x0b1f
    .int        0x0b4a, 0x0b76, 0x0ba3, 0x0bd1, 0x0bff, 0x0c2e, 0x0c5d, 0x0c8e
    .int        0x0cbf, 0x0cf1, 0x0d23, 0x0d56, 0x0d8a, 0x0dbf, 0x0df5, 0x0e2b
    .int        0x0e62, 0x0e9a, 0x0ed3, 0x0f0c, 0x0f47, 0x0f82, 0x0fbe, 0x0fff
dimmer_values_shifted:
    .int        0x0000, 0x0040, 0x0060, 0x0070, 0x0090, 0x00b0, 0x00c0, 0x00e0
    .int        0x00f0, 0x0110, 0x0130, 0x0140, 0x0160, 0x0180, 0x01a0, 0x01b0
    .int        0x01d0, 0x01f0, 0x0210, 0x0230, 0x0250, 0x0270, 0x0290, 0x02b0
    .int        0x02d0, 0x02f0, 0x0310, 0x0330, 0x0350, 0x0370, 0x03a0, 0x03c0
    .int        0x03e0, 0x0410, 0x0430, 0x0450, 0x0480, 0x04a0, 0x04d0, 0x04f0
    .int        0x0520, 0x0550, 0x0570, 0x05a0, 0x05d0, 0x05f0, 0x0620, 0x0650
    .int        0x0680, 0x06b0, 0x06e0, 0x0710, 0x0740, 0x0770, 0x07a0, 0x07e0
    .int        0x0810, 0x0840, 0x0880, 0x08b0, 0x08f0, 0x0920, 0x0960, 0x0990
    .int        0x09d0, 0x0a10, 0x0a50, 0x0a80, 0x0ac0, 0x0b00, 0x0b40, 0x0b80
    .int        0x0bd0, 0x0c10, 0x0c50, 0x0c90, 0x0ce0, 0x0d20, 0x0d70, 0x0db0
    .int        0x0e00, 0x0e50, 0x0ea0, 0x0ee0, 0x0f30, 0x0f80, 0x0fd0, 0x1030
    .int        0x1080, 0x10d0, 0x1130, 0x1180, 0x11e0, 0x1230, 0x1290, 0x12f0
    .int        0x1350, 0x13b0, 0x1410, 0x1470, 0x14d0, 0x1540, 0x15a0, 0x1610
    .int        0x1670, 0x16e0, 0x1750, 0x17c0, 0x1830, 0x18a0, 0x1910, 0x1980
    .int        0x1a00, 0x1a80, 0x1af0, 0x1b70, 0x1bf0, 0x1c70, 0x1cf0, 0x1d70
    .int        0x1e00, 0x1e80, 0x1f10, 0x1fa0, 0x2030, 0x20c0, 0x2150, 0x21e0
    .int        0x2280, 0x2310, 0x23b0, 0x2450, 0x24f0, 0x2590, 0x2630, 0x26e0
    .int        0x2780, 0x2830, 0x28e0, 0x2990, 0x2a40, 0x2b00, 0x2bb0, 0x2c70
    .int        0x2d30, 0x2df0, 0x2ec0, 0x2f80, 0x3050, 0x3120, 0x31f0, 0x32c0
    .int        0x3390, 0x3470, 0x3550, 0x3630, 0x3710, 0x3800, 0x38e0, 0x39d0
    .int        0x3ac0, 0x3bc0, 0x3cb0, 0x3db0, 0x3eb0, 0x3fb0, 0x40c0, 0x41d0
    .int        0x42e0, 0x43f0, 0x4500, 0x4620, 0x4740, 0x4860, 0x4990, 0x4ac0
    .int        0x4bf0, 0x4d20, 0x4e60, 0x4fa0, 0x50e0, 0x5230, 0x5380, 0x54d0
    .int        0x5630, 0x5780, 0x58f0, 0x5a50, 0x5bc0, 0x5d30, 0x5eb0, 0x6020
    .int        0x61b0, 0x6330, 0x64c0, 0x6650, 0x67f0, 0x6990, 0x6b40, 0x6ce0
    .int        0x6ea0, 0x7050, 0x7210, 0x73e0, 0x75b0, 0x7780, 0x7960, 0x7b40
    .int        0x7d20, 0x7f10, 0x8110, 0x8310, 0x8510, 0x8720, 0x8940, 0x8b60
    .int        0x8d80, 0x8fb0, 0x91f0, 0x9430, 0x9670, 0x98c0, 0x9b20, 0x9d80
    .int        0x9ff0, 0xa260, 0xa4e0, 0xa770, 0xaa00, 0xac90, 0xaf40, 0xb1f0
    .int        0xb4a0, 0xb760, 0xba30, 0xbd10, 0xbff0, 0xc2e0, 0xc5d0, 0xc8e0
    .int        0xcbf0, 0xcf10, 0xd230, 0xd560, 0xd8a0, 0xdbf0, 0xdf50, 0xe2b0
    .int        0xe620, 0xe9a0, 0xed30, 0xf0c0, 0xf470, 0xf820, 0xfbe0, 0xfff0

    .end
