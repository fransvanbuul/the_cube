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
 *
 */

#include <avr/io.h>
#include "cube_sizes.h"

    .macro      spi_send_byte_nowait b:req
    OUT         _SFR_IO_ADDR(SPDR), \b    
    .endm

    .macro      spi_send_waitcomplete b:req
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
    .end
