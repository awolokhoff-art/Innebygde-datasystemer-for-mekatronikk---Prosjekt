#include <Arduino.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <FlexCAN_T4.h>
#include "joystick.h"


/*
 - structen MessageID kan muligens være i en egen h-fil istedenfor i toppen her? 
 - det er generelt mange variabler som kan grupperes i struct. feks en hardwareconfig eller paddle som inneholder "height, width" osv. 
 - mangler fortsatt en reset-funksjon her. Den i chatgpt-koden fungerte ikke


*/

constexpr uint8_t joyClick{19};
constexpr uint8_t joyUp{22};
constexpr uint8_t joyDown{23};     
constexpr uint8_t oledDC{6};
constexpr uint8_t oledCS{10};      
constexpr uint8_t oledReset{5};

constexpr uint8_t screenWidth{128};
constexpr uint8_t screenHeight{64};

struct MessageID
{
  const uint32_t joystickData{25};
  const uint32_t paddlePositionPlayer1{26};
  const uint32_t paddlePositionPlayer2{27};
  const uint32_t ballPosition{56};
  const uint32_t score{57};    // mottar scorePlayer1 (buf0) og scorePlayer2 (buf1)
  const uint32_t resetGame{58};
};

MessageID messageID; 

constexpr uint8_t paddleWidth{4};
constexpr uint8_t paddleHeight{20};
uint8_t paddleXPosition{screenWidth-paddleWidth};
uint8_t paddleYPosition;
uint8_t paddleXPositionOpponent{0}; 
uint8_t paddleYPositionOpponent; 

uint8_t ballXCoordinate; 
uint8_t ballYCoordinate;
constexpr uint8_t ballRadius{3};

uint8_t scorePlayer1;
uint8_t scorePlayer2;
uint8_t winningScore{5};

Adafruit_SSD1306 display(screenWidth, screenHeight, &SPI, oledDC, oledReset, oledCS); 
FlexCAN_T4 < CAN0, RX_SIZE_256, TX_SIZE_16 > can0;
Joystick joystick(joyUp, joyDown, joyClick); 

void updateScore();
void gameOver();
void sendJoystickData();
void readCANInbox(MessageID& messageID);


unsigned long lastSendTime{0};          // lagrer tiden (ms) siden siste sendte melding
const unsigned long sendInterval{100};  // For å ikke spamme med can-meldinger på joystickdata

void setup() 
{
  Serial.begin(9600);
  delay(200);
  can0.begin();
  can0.setBaudRate(250000);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) 
  {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.display();
}


void loop() 
{
  sendJoystickData(); 
  readCANInbox();
  
  display.clearDisplay(); 
  display.fillRect(paddleXPosition, paddleYPosition, paddleWidth, paddleHeight, SSD1306_WHITE); // player1 teensy 
  display.fillRect(0, paddleYPositionOpponent, paddleWidth, paddleHeight, SSD1306_WHITE);       // player2 raspberry
  display.fillCircle(ballXCoordinate, ballYCoordinate, ballRadius, SSD1306_WHITE); 

  updateScore();
  if ( scorePlayer1 >= winningScore || scorePlayer2 >= winningScore )
  { 
    gameOver();
  }
  
  display.display();
}




void sendJoystickData()
{
  CAN_message_t joystickData;
  joystickData.id = messageID.joystickData;
  joystickData.len = 1;
  if ( joystick.joyUp() && millis() - lastSendTime > sendInterval )
  { 
    joystickData.buf[0] = 1;
    can0.write(joystickData);
    lastSendTime = millis(); // lagrer tidspunktet for når meldingen ble sendt
  }
  if ( joystick.joyDown() && millis() - lastSendTime > sendInterval )
  { 
    joystickData.buf[0] = 2;
    can0.write(joystickData);
    lastSendTime = millis();
  }
}


void readCANInbox()
{
  CAN_message_t receivedMessage; 
  can0.read(receivedMessage); 
  if ( receivedMessage.id == messageID.paddlePositionPlayer1 )
  {
    paddleYPosition = receivedMessage.buf[0];
  }

  if ( receivedMessage.id == messageID.paddlePositionPlayer2 ) // for player2 (raspberry)
  {
    paddleYPositionOpponent = receivedMessage.buf[0]; 
  }
  
  if ( receivedMessage.id == messageID.ballPosition )
  {
    ballXCoordinate = receivedMessage.buf[0];
    ballYCoordinate = receivedMessage.buf[1];
  }

  if ( receivedMessage.id == messageID.score )
  {
    scorePlayer1 = receivedMessage.buf[0];
    scorePlayer2 = receivedMessage.buf[1]; 
  }
}


void updateScore()
  {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1); 
    display.setCursor(screenWidth/2 - 20, 0);
    display.print(scorePlayer2);
    display.setCursor(screenWidth/2, 0);
    display.print("-");
    display.setCursor(screenWidth/2 + 20, 0);
    display.print(scorePlayer1);
  }

void gameOver()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(25, 10);
  if ( scorePlayer1 > scorePlayer2 ) {
    display.print("Teensy wins!");
  }
  else {
    display.print("Raspberry wins!");
  }
}

