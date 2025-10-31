#include "ball.h"

Ball::Ball(int8_t xPosition)
  : xPosition_{xPosition}
  , yPosition_{32}
  , xVelocity_{1}
  , yVelocity_{1}
  , radius_{3}  
  , frameTop_{64}  
  , frameBottom_{0}   
  , frameLeft_{0}
  , frameRight_{128}
{}

void Ball::reset()
{
  xPosition_ = 64;
  yPosition_ = 32;
  xVelocity_ = 1; 
  yVelocity_ = 1;
}

void Ball::inMotion() 
{ 
  xPosition_ += xVelocity_;
  yPosition_ += yVelocity_;

  if ( top() >= frameTop_ || bottom() <= frameBottom_ ) // krasj med vegger
  {
    yVelocity_ = -yVelocity_;
  }
}


void Ball::bounce()
{
  xVelocity_ = -xVelocity_;
}


bool Ball::hitPaddle(Paddle& paddle)
{
  if ( paddle.leftSide() > 64 )
  {
    if ( rightSide() >= paddle.leftSide() && // Ballens høyre kant treffer paddlens venstre kant (høyre paddle)
          bottom() >= paddle.top() &&      // Bunnen av ballen ligger innenfor toppen av paddlen
             top() <= paddle.bottom() )    // Toppen av ballen ligger innenfor bunnen av paddlen
    { return true; }
  }

  else 
  {
    if ( leftSide() <= paddle.rightSide() &&  // Ballens venstre kant treffer paddlens høyre kant (venstre paddle)
              bottom() >= paddle.top() &&        // Bunnen av ballen ligger innenfor toppen av paddlen
                 top() <= paddle.bottom() )      // Toppen av ballen ligger innenfor bunnen av paddlen
    { return true; }
  }

  return false;
}


bool Ball::isOutLeft()
{
  if (leftSide() < frameLeft_)
  {
    return true; 
  }
  else
  {
    return false;
  }
}


bool Ball::isOutRight()
{
  if (rightSide() > frameRight_)
  {
    return true; 
  }
  else 
  {
    return false;
  }
}


void Ball::draw(Adafruit_SSD1306& display)
{
  display.fillCircle(xPosition_, yPosition_, radius_, SSD1306_WHITE);
}