#include "cube_shared.h"

static uint8_t pos0_x = 0, pos0_z = 0, pos1_x = 1, pos1_z = 0, pos2_x = 2, pos2_z = 0;
static void snake_nextimage();
static void snake_delay();

void snake_loop() {
  snake_nextimage();
  snake_delay();
}

static void snake_nextimage() {
  bool done = false;
  while(!done) {
    uint8_t candidate_x = pos0_x;
    uint8_t candidate_z = pos0_z;
    done = true;
    switch(random(4)) {
      case 0: candidate_x--; break;
      case 1: candidate_x++; break;
      case 2: candidate_z--; break;
      case 3: candidate_z++; break;
    }
    if(candidate_x > 2 || candidate_z > 2) done = false;
    if(candidate_x == pos1_x && candidate_z == pos1_z) done = false;
    if(done) {
      pos2_x = pos1_x;
      pos2_z = pos1_z;
      pos1_x = pos0_x;
      pos1_z = pos0_z;
      pos0_x = candidate_x;
      pos0_z = candidate_z;
    }
  }
    
  wait_vsync();
  memset(display_mem, 0, SIZE_X*SIZE_Y*SIZE_Z);
  for(int y = 0; y < 3; y++) {
    display_mem[pos0_x][y][pos0_z] = 255;
    display_mem[pos1_x][y][pos1_z] = 127;
    display_mem[pos2_x][y][pos2_z] = 63;
  }

}

static void snake_delay() {
  bool moving = false;
  while(!moving) {
    int freq_in = analogRead(7);
    if(freq_in > 0) {
      moving = true;
      float freq_desired = ((float)freq_in)*30.0/1023.0;
      int delayMillis = (int)(1000.0/freq_desired);
      if(delayMillis > 500) delayMillis = 500;
      delay(delayMillis);
    }
  }  
}
