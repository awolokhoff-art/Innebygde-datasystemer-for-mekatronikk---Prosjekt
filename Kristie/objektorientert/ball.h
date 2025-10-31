#ifndef BALL_H
#define BALL_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h> 
#include <Adafruit_GFX.h> 

#include "paddle.h"

class Ball
{
  public:
  Ball(int8_t xPosition);

  void reset();
  void inMotion();
  void bounce();
  bool hitPaddle(Paddle& paddle);
  bool isOutLeft();
  bool isOutRight();
  int8_t leftSide()  {return xPosition_ - radius_;}
  int8_t rightSide() {return xPosition_ + radius_;}
  int8_t bottom()    {return yPosition_ - radius_;}
  int8_t top()       {return yPosition_ + radius_;}
  void draw(Adafruit_SSD1306& display);

  private:
  int8_t xPosition_;
  int8_t yPosition_; 
  int8_t xVelocity_; 
  int8_t yVelocity_;
  const uint8_t radius_; 
  const uint8_t frameTop_;
  const uint8_t frameBottom_;
  const uint8_t frameLeft_;
  const uint8_t frameRight_;
};
#endif