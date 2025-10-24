#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

const int JOY_RIGHT = 17;
const int JOY_LEFT  = 18;
const int JOY_CLICK = 19;
const int JOY_UP    = 22;
const int JOY_DOWN  = 23;

#define OLED_DC         6
#define OLED_CS         10
#define OLED_RESET      5
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// --- Platevariabler ---
int platePosition = 22;       // startposisjon for platen
const int plateHeight = 20; // høyde på platen
const int plateWidth = 4; 

const int groupNumber = 6; 
const int IDPlatePosition = groupNumber + 20; 


int xBall = 10;     // Ball X position
int yBall = 10;     // Ball Y position
int xVelocity = 1;      // X velocity
int yVelocity = 1;      // Y velocity
int const ballRadius = 3;  // Ball ballRadius



void setup() 
{
    Serial.begin(9600);
    delay(200); // sjekk om denne er nødvendig. Teensy booter raskere enn pcen; for å unngå at oppstartstekst forsvinner før man rekker å se den på pc-skjermen

    // INPUT_PULLUP: aktiverer en intern motstand som holder pinnen høy når ingen trykker (for å unngå flimmer i signalet)
    pinMode(JOY_LEFT, INPUT_PULLUP);
    pinMode(JOY_RIGHT, INPUT_PULLUP);
    pinMode(JOY_UP, INPUT_PULLUP);
    pinMode(JOY_DOWN, INPUT_PULLUP);
    pinMode(JOY_CLICK, INPUT_PULLUP);
    Serial.println("Klar til å lese joystick!"); // serial print sender tekst/tall fra teensy til pc

    // starter oled skjerm
  if (!display.begin(SSD1306_SWITCHCAPVCC)) 
  {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.display();
  Serial.println("Skjerm OK. Klar til bruk!");
  
}

void loop() 
{

    // les joystick og oppdater posisjon
    if (digitalRead(JOY_UP) == LOW && platePosition > 0) 
    {
        platePosition -= 2;  // flytt opp
    }

    if (digitalRead(JOY_DOWN) == LOW && platePosition < SCREEN_HEIGHT - plateHeight) 
    {
        platePosition += 2;  // flytt ned
    }

    // tegner platen og ball
    display.clearDisplay();
    display.fillRect(SCREEN_WIDTH - plateWidth, platePosition, plateWidth, plateHeight, SSD1306_WHITE);
    display.fillCircle(xBall, yBall, ballRadius, SSD1306_WHITE);



    
    xBall += xVelocity;
    yBall += yVelocity;

    if (xBall <= ballRadius || xBall >= SCREEN_WIDTH - ballRadius) 
    {
    xVelocity = -xVelocity;
    }

    if (yBall <= ballRadius || yBall >= SCREEN_HEIGHT - ballRadius) 
    {
    yVelocity = -yVelocity;
    }
    

    
    // kollisjon med plate
    int plateX = SCREEN_WIDTH - plateWidth;
    if (xBall + ballRadius >= plateX && 
    yBall >= platePosition && 
    yBall <= platePosition + plateHeight) 
{
    xVelocity = -xVelocity;  // spretter tilbake
    xBall = plateX - ballRadius; // flytt ballen utenfor platen for å unngå “klemming”
}
    

    display.display();

    delay(15); // for jevn bevegelse
}

