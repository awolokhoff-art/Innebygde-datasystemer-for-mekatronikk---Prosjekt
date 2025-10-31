#include "pingponggame.h"

PingPongGame::PingPongGame(Ball& ball)
  : ball_{ball}
  , gameRunning_{false}
{}

/*
void PingPongGame::startup()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 25);
  display.println("Press joystick to start");
  display.display();

  // vent til spilleren trykker joystick
  while (digitalRead(JOY_CLICK) == HIGH) 
  {}

  delay(300);
}*/


void PingPongGame::gameOver(Adafruit_SSD1306& display) // når ballen toucher sidekantene (går forbi paddle)
{
  if ( ball_.isOutLeft() || ball_.isOutRight() ) // 
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 25);
    display.println("GAME OVER");
    display.display();
  }
}


/* spill-logikk som skal styres
  -> spilleren som beveger platen først er master
  -> hvis master: send ballposisjon
  -> hvis slave: motta ballposisjon
  -> start spillet
  -> hvis spiller_x mister ballen: GAME OVER, spiller_y vinner
  -> tilbake til initial

*/