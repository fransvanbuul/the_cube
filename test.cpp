#include "cube_shared.h"

static Vector point;
  
void test_loop() {
  memset(display_mem, 0, SIZE_X*SIZE_Y*SIZE_Z);
  point.pset(0xFF);
  if(++point.x >= SIZE_X) {
    point.x = 0;
    if(++point.y >= SIZE_Y) {
      point.y = 0;
      if(++point.z >= SIZE_Z) {
        point.z = 0;
      }
    }
  }
  delay(2000);
}
