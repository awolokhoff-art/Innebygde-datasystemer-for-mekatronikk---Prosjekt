#ifndef PING_PONG_GAME_H
#define PING_PONG_GAME_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h> 
#include <Adafruit_GFX.h> 
#include "joystick.h"
#include "ball.h"


class PingPongGame
{
  public:
  PingPongGame(Ball& ball);

  Ball& ball_;

  void startup();
  void gameOver(Adafruit_SSD1306& display);

  private:
  bool gameRunning_;
};

#endif