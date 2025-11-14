#include "joystick.h"

Joystick::Joystick(const uint8_t up, const uint8_t down)
  : up_{up}
  , down_{down}
{}


void Joystick::init() // er denne nødvendig? Hvis man lager objektet i setup(), forsvinner objektet etter det er laget. men må pins i det heletatt settes i setup? 
{
  pinMode(up_, INPUT_PULLUP);
  pinMode(down_, INPUT_PULLUP);
  Serial.println("Klar til å lese joystick!");
}

bool Joystick::joyUP()
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


bool Joystick::joyDOWN()
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
