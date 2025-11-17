#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>

class Joystick
{
  public:
  Joystick(const uint8_t up, const uint8_t down, const uint8_t click);

  bool joyUp();
  bool joyDown();
  bool joyClick();

  private:
  const uint8_t up_;
  const uint8_t down_;
  const uint8_t click_;

};
#endif 