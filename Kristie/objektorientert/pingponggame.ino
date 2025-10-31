#include <Arduino.h>            // grunnbibliotek. datatyper, pinmode(), digitalread() osv.
#include <Adafruit_GFX.h>       // tegnefunksjoner
#include <Adafruit_SSD1306.h>   // driver for OLED
#include <SPI.h>                // SPI-kommunikasjon for OLED
#include <FlexCAN_T4.h>

#include "joystick.h"
#include "paddle.h"
#include "ball.h"
#include "pingponggame.h"
#include "canbushandler.h"

constexpr uint8_t joyUp{22};
constexpr uint8_t joyDown{23};     
constexpr uint8_t oledDC{6};
constexpr uint8_t oledCS{10};      
constexpr uint8_t oledReset{5};
constexpr uint8_t screenWidth{128};
constexpr uint8_t screenHeight{64};


// objekter
FlexCAN_T4 < CAN0, RX_SIZE_256, TX_SIZE_16 > can0; // Can0-objekt
CanBusHandler canBusHandler(can0, 6, 3);
Adafruit_SSD1306 display(screenWidth, screenHeight, &SPI, oledDC, oledReset, oledCS); 
Joystick joystick(joyUp, joyDown);   
Paddle myPaddle(124); 
Paddle enemyPaddle(0); 
Ball ball(64);
PingPongGame pingPongGame(ball, myPaddle, joystick, display);


CAN_message_t msgEnemyPosition;
uint8_t enemyGroupNr = 23;



void setup() 
{
  Serial.begin(9600);
  can0.begin();
  can0.setBaudRate(500000);

  // starter oled skjerm
  /*if (!display.begin(SSD1306_SWITCHCAPVCC)) 
  {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }*/ /////////////////////////////////////////////////////////// Trenger ikke dette når det er pakket inn i pingPongGame.showStartScreen();

  //display.clearDisplay(); // superviktig at denne kommer etter det over^ //denne må være med hvis showStartScreen(); blir fjernet
  //display.display();      // superviktig at denne kommer etter det over^ //denne må være med hvis showStartScreen(); blir fjernet
  joystick.init();
  ball.reset();
  pingPongGame.showStartScreen();
  
}


void loop() 
{
  if ( !pingPongGame.gameRunning_ )
  {
    pingPongGame.showStartScreen();
    return;
  }

  display.clearDisplay();
  myPaddle.updatePositionFromJoystick(joystick);
  myPaddle.draw(display);
  canBusHandler.sendPaddlePosition(myPaddle);

  if ( can0.read(msgEnemyPosition) ) 
  { 
    if (msgEnemyPosition.id == enemyGroupNr ) //sjekker at meldingen er ang. paddle posisjon
    {
      enemyPaddle.updatePositionFromCAN(msgEnemyPosition);
    }
  }
  enemyPaddle.draw(display);
  ball.draw(display);
  ball.inMotion();

  if (ball.hitPaddle(myPaddle) || ball.hitPaddle(enemyPaddle))
  {
    ball.bounce();
  }
 
 
  if (ball.isOutRight() || ball.isOutLeft())
  {
    pingPongGame.gameOver();
  }
 
  display.display();
  delay(15); // for jevn bevegelse av ball og plate 
}












