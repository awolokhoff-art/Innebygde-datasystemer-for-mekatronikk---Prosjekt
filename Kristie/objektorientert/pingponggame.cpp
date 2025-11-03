#include "pingponggame.h"

PingPongGame::PingPongGame(Ball& ball, Paddle& paddle, Joystick& joystick, Adafruit_SSD1306& display)
  : ball_{ball}
  , paddle_{paddle}
  , joystick_{joystick}
  , display_{display}
  , gameRunning_{false}
{}


void PingPongGame::showStartScreen()
{
  if (!display_.begin(SSD1306_SWITCHCAPVCC)) 
  {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }

  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(15, 25);
  display_.println("Press joystick to start");
  display_.display();

  // vent til spilleren trykker joystick
  while ( joystick_.joyUP() == false && joystick_.joyDOWN() == false ) 
  {}
  delay(300); 
  gameRunning_ = true;
  ball_.reset();
}


void PingPongGame::gameOver() // når ballen toucher sidekantene (går forbi paddle)
{
    display_.clearDisplay();
    display_.setTextSize(2);
    display_.setTextColor(SSD1306_WHITE);
    display_.setCursor(20, 25);
    display_.println("GAME OVER");
    display_.display();
    delay(2000);
    ball_.reset();
}


/* spill-logikk som skal styres
  -> spilleren som beveger platen først er master
  -> hvis master: send ballposisjon
  -> hvis slave: motta ballposisjon
  -> start spillet
  -> hvis spiller_x mister ballen: GAME OVER, spiller_y vinner
  -> tilbake til initial

*/