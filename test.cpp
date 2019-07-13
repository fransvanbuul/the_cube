#include "cube_shared.h"

static uint8_t x = 0, y = 0, z = 0;
  
void test_loop() {
  memset(display_mem, 0xFF, SIZE_X*SIZE_Y*SIZE_Z);
  display_mem[x][y][z] = 0x0;
  if(++x >= 3) { 
    x = 0;
    if(++y >= 3) {
      y = 0;
      if(++z >= 3) {
        z = 0;
      }
    } 
  }
  delay(250);
}
