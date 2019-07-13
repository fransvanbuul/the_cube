#include "cube_shared.h"

void liquid_loop() {
  int analog_in = analogRead(7);
  mpu6050.update();
  Vector acc = Vector(0x40*mpu6050.getAccX(), -0x40*mpu6050.getAccY(), 0x40*mpu6050.getAccZ());
  int16_t c = Vector(1, 1, 1) * acc + ((analog_in - 512)/5);
  for(int8_t z = 0; z < 3; z++) {
    for(int8_t y = 0; y < 3; y++) {
      for(int8_t x = 0; x < 3; x++) {
        Vector p = Vector(x, y, z);
        int16_t d = p * acc - c;
        if(d < -32) {
          p.pset(0xFF);
        } else if(d >= 32) {
          p.pset(0);
        } else {
          p.pset(0xFF - 4*(d + 32));
        }
      }
    }
  }
}
