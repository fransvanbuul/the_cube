#ifndef CUBE_SHARED_H
#define CUBE_SHARED_H

#include <Arduino.h>
#include <stdint.h>

#define SIZE_X               3
#define SIZE_Y               3
#define SIZE_Z               8
#define SIZE_TLC5940         1

#if (SIZE_X*SIZE_Y) > (SIZE_TLC5940*16)
#error Not enough TCL5940s
#endif

extern uint8_t display_mem[SIZE_X][SIZE_Y][SIZE_Z];  

void wait_vsync();
void snake_loop();

#endif // CUBE_SHARED_H
