#ifndef CUBE_SIZES_H
#define CUBE_SIZES_H

#define SIZE_X               3
#define SIZE_Y               3
#define SIZE_Z               3
#define SIZE_TLC5940         1

#if (SIZE_X*SIZE_Y) > (SIZE_TLC5940*16)
#error Not enough TCL5940s
#endif

#endif // CUBE_SIZES_H
