#ifndef PING_PONG_GAME_H
#define PING_PONG_GAME_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h> 
#include <Adafruit_GFX.h> 
#include "ball.h"
#include "paddle.h"
#include "joystick.h"


class PingPongGame
{
  public:
  PingPongGame(Ball& ball, Paddle& paddle, Joystick& joystick, Adafruit_SSD1306& display);

  Ball& ball_;
  Paddle& paddle_;
  Joystick& joystick_;
  Adafruit_SSD1306& display_;
  bool gameRunning_;

  void showStartScreen();
  void gameOver();

  private:
  
};

#endif