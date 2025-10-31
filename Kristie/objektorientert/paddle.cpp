#include "paddle.h"

Paddle::Paddle(const uint8_t xPosition)
  : xPosition_{xPosition}
  , yPosition_{22}
  , height_{20}
  , width_{4}
  , minPosition_{0}
  , maxPosition_{44}
{}


void Paddle::updatePositionFromJoystick(Joystick& joystick)
{
  if (joystick.joyUP() && yPosition_ > minPosition_)
  {
    yPosition_ -= 2; // flytt opp
  }
  if (joystick.joyDOWN() && yPosition_ < maxPosition_)
  {
    yPosition_ += 2; // flytt ned
  }
}


void Paddle::updatePositionFromCAN(CAN_message_t& msgPosition)
{
  if (msgPosition.len < 1) // sjekker at meldingen faktisk inneholder minst 1 byte
  {
    return;  // ingen data
  }

  receivedPosition_ = msgPosition.buf[0];

  if ( receivedPosition_ < minPosition_ )
  {
    receivedPosition_ = minPosition_;
  }
  else if ( receivedPosition_ > maxPosition_ )
  {
    receivedPosition_ = maxPosition_;
  }
  else
  {
    yPosition_ = receivedPosition_; 
  }
}


void Paddle::draw(Adafruit_SSD1306& display)
{
  display.fillRect(xPosition_, yPosition_, width_, height_, SSD1306_WHITE);
}




