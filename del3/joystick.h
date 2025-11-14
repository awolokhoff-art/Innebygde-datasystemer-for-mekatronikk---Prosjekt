#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>

class Joystick
{
  public:
  Joystick(const uint8_t up, const uint8_t down);

  void init(); 
  bool joyUP();
  bool joyDOWN();

  private:
  const uint8_t up_;
  const uint8_t down_;
};
#endif 