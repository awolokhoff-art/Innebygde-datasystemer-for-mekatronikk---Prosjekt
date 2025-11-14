#include <Arduino.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <FlexCAN_T4.h>
#include "joystick.h"

constexpr uint8_t joyUp{22};
constexpr uint8_t joyDown{23};     
constexpr uint8_t oledDC{6};
constexpr uint8_t oledCS{10};      
constexpr uint8_t oledReset{5};
constexpr uint8_t screenWidth{128};
constexpr uint8_t screenHeight{64};

Adafruit_SSD1306 display(screenWidth, screenHeight, &SPI, oledDC, oledReset, oledCS); 
FlexCAN_T4 < CAN0, RX_SIZE_256, TX_SIZE_16 > can0;
Joystick joystick(joyUp, joyDown); 

// For å ikke spamme med can-meldinger på joystickdata
unsigned long lastSendTime{0};          // lagrer tiden (ms) siden siste sendte melding
const unsigned long sendInterval{100};

void setup() 
{
  Serial.begin(9600); // stod på 115200, anbefalt for rasp Pi? 9600 er vanlig for sensorer? 
  delay(200);

  can0.begin();
  can0.setBaudRate(250000);  // Samme bitrate som Pi

  if (!display.begin(SSD1306_SWITCHCAPVCC)) 
  {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.display();

  joystick.init();

}

void loop() 
{
  /////////////////////////////////// sender melding dersom joysticken blir flyttet på: ///////////////////////////////// 
  CAN_message_t sendJoystickData;
  sendJoystickData.id = 8;
  sendJoystickData.len = 1;
  if ( joystick.joyUP() && millis() - lastSendTime > sendInterval )
  { 
    sendJoystickData.buf[0] = 1;
    can0.write(sendJoystickData);
    lastSendTime = millis(); // lagrer tidspunktet for når meldingen ble sendt
  }
  if ( joystick.joyDOWN() && millis() - lastSendTime > sendInterval )
  { 
    sendJoystickData.buf[0] = 0;
    can0.write(sendJoystickData);
    lastSendTime = millis();
  } /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // tegne plate basert på sensordata

  

  
  


}
