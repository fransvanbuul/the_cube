#ifndef CUBE_SHARED_H
#define CUBE_SHARED_H

#include <Arduino.h>
#include <Print.h>
#include <stdint.h>
#include <MPU6050_tockn.h>

#define SIZE_X               8
#define SIZE_Y               8
#define SIZE_Z               8
#define SIZE_TLC5940         4

#if (SIZE_X*SIZE_Y) > (SIZE_TLC5940*16)
#error Not enough TCL5940s
#endif

extern uint8_t display_mem[SIZE_X][SIZE_Y][SIZE_Z];  
extern MPU6050 mpu6050;

class Vector: public Printable {
public:
  int8_t x, y, z;
  Vector(int8_t x0 = 0, int8_t y0 = 0, int8_t z0 = 0) { x = x0; y = y0; z = z0; }
  Vector(const Vector& v) { x = v.x; y = v.y; z = v.z; }
  void pset(uint8_t value) const { display_mem[x][y][z] = value; }
  bool isInCube() const { return x >= 0 && y >= 0 && z >= 0 && x < SIZE_X && y < SIZE_Y && z < SIZE_Z; }

  bool operator==(const Vector& v) const { return x == v.x && y == v.y && z == v.z; }
  bool operator!=(const Vector& v) const { return x != v.x || y != v.y || z != v.z; }
  Vector& operator+=(const Vector& v) { x += v.x; y += v.y; z += v.z; return *this; }
  Vector operator+(const Vector& v) const { return Vector(x + v.x, y + v.y, z + v.z); }
  Vector& operator-=(const Vector& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
  Vector operator-(const Vector& v) const { return Vector(x - v.x, y - v.y, z - v.z); }
  Vector operator-() const { return Vector(-x, -y, -z); }
  int16_t operator*(const Vector& v) const { return x * v.x + y * v.y + z * v.z; }
  Vector& operator*=(int8_t m) { x *= m; y *= m; z *= m; return *this; }
  Vector operator*(int8_t m) const { return Vector(x * m, y * m, z * m); }
  Vector& operator/=(int8_t m) { x /= m; y /= m; z /= m; return *this; }
  Vector operator/(int8_t m) const { return Vector(x / m, y / m, z / m); }

  virtual size_t printTo(Print& p) const { 
    size_t n = 0; 
    n += p.print('('); 
    n += p.print(x); n += p.print(','); 
    n += p.print(y); n += p.print(','); 
    n += p.print(z); n += p.print(')'); 
    return n; 
  }
};

void snake_loop();
void snake3d_loop();
void test_loop();
void liquid_loop();

#endif // CUBE_SHARED_H
