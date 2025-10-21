// Simple joystick + OLED test
// Cleaned up by ChatGPT (original: K. M. Knausgård 2023-10-21)

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

// ------------------ PIN CONFIG ------------------
#define JOY_LEFT    18
#define JOY_RIGHT   17
#define JOY_CLICK   19
#define JOY_UP      22
#define JOY_DOWN    23

#define OLED_DC     6
#define OLED_CS     10
#define OLED_RESET  5

// ------------------ OLED SETTINGS ------------------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ------------------ SETUP ------------------
void setup() {
  Serial.begin(9600);

  pinMode(JOY_LEFT, INPUT_PULLUP);
  pinMode(JOY_RIGHT, INPUT_PULLUP);
  pinMode(JOY_UP, INPUT_PULLUP);
  pinMode(JOY_DOWN, INPUT_PULLUP);
  pinMode(JOY_CLICK, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Joystick + OLED test"));
  display.display();
  delay(1500);
}

// ------------------ LOOP ------------------
void loop() {
  bool left   = !digitalRead(JOY_LEFT);
  bool right  = !digitalRead(JOY_RIGHT);
  bool up     = !digitalRead(JOY_UP);
  bool down   = !digitalRead(JOY_DOWN);
  bool click  = !digitalRead(JOY_CLICK);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Joystick status:"));
  display.println();

  display.print(F("Up:     "));   display.println(up ? F("YES") : F("no"));
  display.print(F("Down:   ")); display.println(down ? F("YES") : F("no"));
  display.print(F("Left:   ")); display.println(left ? F("YES") : F("no"));
  display.print(F("Right:  ")); display.println(right ? F("YES") : F("no"));
  display.print(F("Click:  ")); display.println(click ? F("YES") : F("no"));

  display.display();

  // Optional serial debug
  Serial.print("U:"); Serial.print(up);
  Serial.print(" D:"); Serial.print(down);
  Serial.print(" L:"); Serial.print(left);
  Serial.print(" R:"); Serial.print(right);
  Serial.print(" C:"); Serial.println(click);

  delay(100);
}
