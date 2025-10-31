#ifndef PADDLE_H
#define PADDLE_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>   // driver for OLED
#include <Adafruit_GFX.h>       // tegnefunksjoner
#include <FlexCAN_T4.h>
#include "joystick.h"

class Paddle
{
  public:
  Paddle(const uint8_t xPosition); 

  int8_t top()       {return yPosition_;} 
  int8_t bottom()    {return yPosition_ + height_;}    
  int8_t leftSide()  {return xPosition_;}
  int8_t rightSide() {return xPosition_ + width_;}

  void updatePositionFromJoystick(Joystick& joystick);
  void updatePositionFromCAN(CAN_message_t& msgPosition);
  void draw(Adafruit_SSD1306& display);

  private: 
  const uint8_t xPosition_; 
  uint8_t yPosition_;
  const uint8_t height_;
  const uint8_t width_;  
  const uint8_t minPosition_;
  const uint8_t maxPosition_;
  uint8_t receivedPosition_;

};
#endif



