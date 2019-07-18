#ifndef CUBE_SIZES_H
#define CUBE_SIZES_H

#define SIZE_X               8
#define SIZE_Y               8
#define SIZE_Z               8
#define SIZE_TLC5940         4

#if SIZE_TLC5940 < 1
#error Number of TCL5940s must be at least 1
#endif

#if (SIZE_X*SIZE_Y) > (SIZE_TLC5940*16)
#error Not enough TCL5940s
#endif

#endif // CUBE_SIZES_H
