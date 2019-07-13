#ifndef CUBE_WIRING_H
#define CUBE_WIRING_H

#define PWM_COUNT            4096
#define pin_high(port, pin)  port |= _BV(pin)
#define pin_low(port, pin)   port &= ~_BV(pin)

#define GSCLK                3
#define DIAG                 2
#define DIAG_PORT            PORTD
#define DIAG_PIN             PD2
#define XLAT                 8
#define XLAT_PORT            PORTB
#define XLAT_PIN             PB0
#define BLANK                9
#define BLANK_PORT           PORTB
#define BLANK_PIN            PB1
#define VPRG                 10
#define VPRG_PORT            PORTB
#define VPRG_PIN             PB2

#define LAYER_0              4
#define LAYER_0_PORT         PORTD
#define LAYER_0_PIN          PD4
#define LAYER_1              5
#define LAYER_1_PORT         PORTD
#define LAYER_1_PIN          PD5
#define LAYER_2              6
#define LAYER_2_PORT         PORTD
#define LAYER_2_PIN          PD6
#define LAYER_3              7
#define LAYER_3_PORT         PORTD
#define LAYER_3_PIN          PD7
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

#endif // CUBE_WIRING_H
