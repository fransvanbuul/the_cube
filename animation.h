#ifndef ANIMATION_H
#define ANIMATION_H

#include "cube_wiring.h"

extern uint8_t display_mem[SIZE_X][SIZE_Y][SIZE_Z];  
extern void wait_vsync();

class Animation {

public:
  virtual void loop() = 0;
  
};

#endif // ANIMATION_H
