#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <FlexCAN_T4.h>

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

const int groupNumber = 6; 
const int idPlatePosition = groupNumber + 20; 

FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;



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

  Can0.begin();
  Can0.setBaudRate(500000);  // 500 kbit/s, standard hastighet
  Serial.println("CAN-bus startet!");
  
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


    display.clearDisplay();
    display.fillRect(SCREEN_WIDTH - 4, platePosition, 4, plateHeight, SSD1306_WHITE); // tegner et rektangel
    display.display();

  // sender en CAN-melding
  CAN_message_t msgPlatePosition;
  msgPlatePosition.id = idPlatePosition;
  msgPlatePosition.len = 1;          
  msgPlatePosition.buf[0] = platePosition;
  Can0.write(msgPlatePosition);

    delay(30); // for jevn bevegelse
}







