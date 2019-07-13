#include "cube_shared.h"

static Vector points[4];
static void snake3d_nextimage();
static void snake3d_delay();
  
void snake3d_loop() {
  snake3d_nextimage();
  snake3d_delay();
}

static void snake3d_nextimage() {
  bool done = false;
  while(!done) {
    Vector candidate(points[0]);
    switch(random(6)) {
      case 0: candidate += Vector(-1,  0,  0); break;
      case 1: candidate += Vector(+1,  0,  0); break;
      case 2: candidate += Vector( 0, -1,  0); break;
      case 3: candidate += Vector( 0, +1,  0); break;
      case 4: candidate += Vector( 0,  0, -1); break;
      case 5: candidate += Vector( 0,  0, +1); break;
    }
    if(candidate.isInCube() && candidate != points[1] && candidate != points[2] && candidate != points[3]) {
      done = true;
      points[3] = points[2];
      points[2] = points[1];
      points[1] = points[0];
      points[0] = candidate;
    }
  }
    
  memset(display_mem, 0, SIZE_X*SIZE_Y*SIZE_Z);
  points[0].pset(255);
  points[1].pset(192);
  points[2].pset(128);
  points[3].pset(64);
}

static void snake3d_delay() {
  bool moving = false;
  while(!moving) {
    int freq_in = analogRead(7);
    if(freq_in > 0) {
      moving = true;
      float freq_desired = ((float)freq_in)*30.0/1023.0;
      int delayMillis = (int)(1000.0/freq_desired);
      if(delayMillis > 500) delayMillis = 500;
      delay(delayMillis);
    } else {
      /* Needed to avoid 'overloading' the analog read */
      delay(100);
    }
  }  
}
