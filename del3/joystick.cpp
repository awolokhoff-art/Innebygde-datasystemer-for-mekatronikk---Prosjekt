#include "joystick.h"

Joystick::Joystick(const uint8_t up, const uint8_t down, const uint8_t click)
  : up_{up}
  , down_{down}
  , click_{click}
{
  pinMode(up_, INPUT_PULLUP);
  pinMode(down_, INPUT_PULLUP);
  Serial.println("Klar til å lese joystick!");
}


bool Joystick::joyUp()
{
  if ( digitalRead(up_) == LOW )
  {
    return true; 
  }
  else
  {
    return false; 
  }
}


bool Joystick::joyDown()
{
  if ( digitalRead(down_) == LOW )
  {
    return true; 
  }
  else
  {
    return false; 
  }
}

bool Joystick::joyClick()
{
  if ( digitalRead(click_) == LOW )
  {
    return true;
  }
  else
  {
    return false; 
  }
}
