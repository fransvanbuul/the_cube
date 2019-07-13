#ifndef CUBE_SHARED_H
#define CUBE_SHARED_H

#include <Arduino.h>
#include <stdint.h>

#define SIZE_X               3
#define SIZE_Y               3
#define SIZE_Z               3
#define SIZE_TLC5940         1

#if (SIZE_X*SIZE_Y) > (SIZE_TLC5940*16)
#error Not enough TCL5940s
#endif

extern uint8_t display_mem[SIZE_X][SIZE_Y][SIZE_Z];  

class Point {
private:
  uint8_t xval, yval, zval;
  
public:
  Point() { xval = 0; yval = 0; zval = 0; }
  Point(uint8_t x, uint8_t y, uint8_t z) { xval = x; yval = y; zval = z; }
  Point(const Point& p) { xval = p.xval; yval = p.yval; zval = p.zval; }
  void pset(uint8_t value) const { display_mem[xval][yval][zval] = value; }
  void add(int8_t dx, int8_t dy, int8_t dz) { xval += dx; yval += dy; zval += dz; }
  bool isInCube() const { return xval < SIZE_X && yval < SIZE_Y && zval < SIZE_Z; }
  bool operator==(const Point& p) const { return xval == p.xval && yval == p.yval && zval == p.zval; }
  bool operator!=(const Point& p) const { return xval != p.xval || yval != p.yval || zval != p.zval; }
  double x() const { return xval; }
  double y() const { return yval; }
  double z() const { return zval; }
};

void snake_loop();
void snake3d_loop();
void test_loop();

#endif // CUBE_SHARED_H
