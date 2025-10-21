// OLED Graphics Test Template
// For testing Adafruit_GFX functions like drawRoundRect(), fillRoundRect(), etc.

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
//mindre tekst
#include <Fonts/TomThumb.h> 

// ------------------ OLED CONFIG ------------------
#define OLED_DC     6
#define OLED_CS     10
#define OLED_RESET  5
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);


// -------------- INFO ------------------
//Skjermen:
//Height 64
//Width 128
// 0,0 er oppe i venstre hjørne


//Rounded rectangle: 
//void drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
//void fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);


//tekst

//display.setFont(&TomThumb); dette vil gi mindre tekst

/*
display.setTextSize(1);  // smallest
display.setCursor(0, 0);
display.print("Small text");

display.setTextSize(2);  // double size
display.print("Bigger text");
*/



// ------------------ SETUP ------------------
void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }

display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("OLED Graphics Test"));
  display.display();
  delay(1000);


}

// ------------------ LOOP ------------------
void loop() {
display.clearDisplay();


// Variabler

int imuVerdi = 12;


// Kant rundt skjermen
display.drawRoundRect(0, 0, 128, 64, 10, SSD1306_WHITE);
  

//HalvSirkelen under teksta
display.drawCircle(64, -282, 300, SSD1306_WHITE);



// Teksta
display.setFont();

display.setCursor(14, 5); // når man bruker 64 så starter den å skrive fra klink midten. Må kompansere
display.print(F("MAS245 - Gruppe 6"));


display.setFont(&TomThumb); //dette vil gi mindre tekst

display.setCursor(5, 28); // 
display.print(F("CAN - Statistikk:"));


display.setCursor(5, 38); // 
display.print(F("Antall mottat:"));


display.setCursor(5, 48); // 
display.print(F("IMU Maaling z:"));
display.print(imuVerdi, 2);  // 2 desimaler

display.display();  // Denne va veldig viktig!
  
  
  /*
  // Clear and draw something else for experimentation
  display.clearDisplay();
  display.drawCircle(64, 32, 20, SSD1306_WHITE);
  display.display();
  delay(2000);
  */
}
