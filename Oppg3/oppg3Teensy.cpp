/*
===============================================================================
  TEENSY PONG CLIENT (for Raspberry Pi Server)
  
  Denne koden er en "tynn klient". Den sender kun joystick-input til RSP3
  og mottar all spill-logikk (plateposisjoner, ball, score) tilbake.
===============================================================================
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // Rettet skrivefeil her
#include <SPI.h>
#include <FlexCAN_T4.h>

// ------------------ Hardware ------------------
constexpr int JOY_RIGHT = 17; 
constexpr int JOY_LEFT  = 18; 
constexpr int JOY_CLICK = 19; 
constexpr int JOY_UP    = 22; 
constexpr int JOY_DOWN  = 23; 

constexpr int OLED_DC     = 6;
constexpr int OLED_CS     = 10;
constexpr int OLED_RESET  = 5;
constexpr int SCREEN_WIDTH  = 128;
constexpr int SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);


// ------------------ CAN-konfig (Må matche Server) ------------------
constexpr int groupNumber = 6;

// --- Sending (Input til Server) ---
constexpr int idJoystickP1 = groupNumber + 19; // ID 25

// --- Mottak (State fra Server) ---
constexpr int idPlatePositionP1 = groupNumber + 20; // ID 26
constexpr int idPlatePositionP2 = groupNumber + 21; // ID 27
constexpr int idBallPosition    = groupNumber + 50; // ID 56
constexpr int idGameOver        = groupNumber + 51; // ID 57
constexpr int idResetGame       = groupNumber + 52; // ID 58

FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;

// ------------------ Spillvariabler (Styres av Server) ------------------
int platePosition = 22;       // P1 (Høyre)
int remotePlatePosition = 22; // P2 (Venstre)
int xBall = 64;
int yBall = 32;
int scoreP1 = 0;
int scoreP2 = 0;

// --- FIKS: Disse manglet i koden din ---
const int plateHeight = 20;
const int plateWidth = 4;
const int ballRadius = 3;
// ---------------------------------------

// ------------------ Spillets tilstand ------------------
const int WINNING_SCORE = 5; 
bool isGameOver = false;

// For å unngå CAN-spam (sender kun ved endring)
int currentMoveState = 0; // 0 = Stille, 1 = Opp, 2 = Ned
int lastMoveState = -1;   


// ============================================================================
// SETUP
// ============================================================================
void setup()
{
  Serial.begin(9600);
  delay(200);

  pinMode(JOY_UP,    INPUT_PULLUP);
  pinMode(JOY_DOWN,  INPUT_PULLUP);
  pinMode(JOY_CLICK, INPUT_PULLUP);

  // Start skjerm
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("ERROR: display.begin() failed."));
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 28);
  display.print("Kobler til server...");
  display.display();

  // Start CAN
  Can0.begin();
  Can0.setBaudRate(250000);
  Serial.println("CAN-bus startet!");
}

// ============================================================================
// Funksjons-prototyper
// ============================================================================
void handleJoystickInput();
void receiveCANMessages();
void drawDisplay();
void sendCANMessages();
void drawGameOverScreen();
void resetGame(); 


// ============================================================================
// HOVEDLØKKE
// ============================================================================
void loop()
{
  if (isGameOver) {
    // --- GAME OVER MODUS ---
    drawGameOverScreen();

    // Hvis P1 trykker Reset (Joystick-klikk)
    if (digitalRead(JOY_CLICK) == LOW) {
      CAN_message_t resetMsg;
      resetMsg.id = idResetGame;
      resetMsg.len = 1;
      resetMsg.buf[0] = 1; 

      // Send meldingen 3 ganger for å være sikker på at Pi mottar den
      for(int i=0; i<3; i++) {
         Can0.write(resetMsg);
         delay(5);
      }
      
      // Vent til knappen slippes
      while(digitalRead(JOY_CLICK) == LOW) delay(10);

      // --- VIKTIG ENDRING ---
      // Vi kjører IKKE resetGame() her lenger.
      // Vi viser tekst og venter på at Raspberry Pi svarer.
      display.fillRect(0, 50, SCREEN_WIDTH, 14, SSD1306_BLACK); // Tøm bunnen av skjermen
      display.setCursor(10, 54);
      display.print("Venter pa server...");
      display.display();
    }
    
    // Denne funksjonen vil nå motta reset-signalet fra Pi-en
    // og kjøre resetGame() automatisk når Pi-en er klar.
    receiveCANMessages(); 

  } else {
    // --- SPILL MODUS ---
    handleJoystickInput(); // Les knapper
    sendCANMessages();     // Send til RSP3 hvis endring
    receiveCANMessages();  // Motta posisjoner fra RSP3
    drawDisplay();         // Tegn skjerm
  }

  delay(10); // 100 Hz oppdatering
}
// ============================================================================
// HJELPEFUNKSJONER
// ============================================================================

void handleJoystickInput() {
  if (digitalRead(JOY_UP) == LOW) {
    currentMoveState = 1; // Opp
  }
  else if (digitalRead(JOY_DOWN) == LOW) {
    currentMoveState = 2; // Ned
  }
  else {
    currentMoveState = 0; // Stille
  }
}

void sendCANMessages() {
  if (currentMoveState == lastMoveState) {
    return; // Ikke spam nettet hvis ingen endring
  }

  CAN_message_t joyMsg;
  joyMsg.id = idJoystickP1; // ID 25
  joyMsg.len = 1;
  joyMsg.buf[0] = currentMoveState;
  Can0.write(joyMsg);

  lastMoveState = currentMoveState;
}

void receiveCANMessages() {
  CAN_message_t rxMsg;
  while (Can0.read(rxMsg))
  {
    if (rxMsg.id == idPlatePositionP1) {
      platePosition = rxMsg.buf[0]; // Min posisjon (fra server)
    }
    else if (rxMsg.id == idPlatePositionP2) { 
      remotePlatePosition = rxMsg.buf[0]; // Motstander
    }
    else if (rxMsg.id == idBallPosition) { 
      xBall = rxMsg.buf[0];
      yBall = rxMsg.buf[1];
    }
    else if (rxMsg.id == idGameOver) { 
      scoreP1 = rxMsg.buf[0];
      scoreP2 = rxMsg.buf[1];
      if (scoreP1 >= WINNING_SCORE || scoreP2 >= WINNING_SCORE) {
        isGameOver = true;
      }
    }
    else if (rxMsg.id == idResetGame && rxMsg.buf[0] == 1 && isGameOver) {
      resetGame();
    }
  }
}

void drawDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Score
  display.setTextSize(1); 
  display.setCursor(SCREEN_WIDTH/2 - 45, 0);
  if(scoreP2 < 10) display.print("0");
  display.print(scoreP2);
  display.setCursor(SCREEN_WIDTH/2 - 6, 0);
  display.print("-");
  display.setCursor(SCREEN_WIDTH/2 + 15, 0);
  if(scoreP1 < 10) display.print("0");
  display.print(scoreP1);

  // Info
  display.setTextSize(1);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print("P1 (Teensy)");

  // Plater og Ball
  display.fillRect(0, remotePlatePosition, plateWidth, plateHeight, SSD1306_WHITE);
  display.fillRect(SCREEN_WIDTH - plateWidth, platePosition, plateWidth, plateHeight, SSD1306_WHITE);
  display.fillCircle(xBall, yBall, ballRadius, SSD1306_WHITE);

  display.display();
}

void drawGameOverScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  bool jegVant = (scoreP1 >= WINNING_SCORE);

  display.setTextSize(2);
  if (jegVant) {
    display.setCursor(25, 10);
    display.print("DU VANT!");
  } else {
    display.setCursor(30, 10);
    display.print("DU TAPTE");
  }

  display.setTextSize(1);
  display.setCursor(20, 40);
  display.print("Trykk for nytt");
  display.setCursor(45, 50);
  display.print("spill");
  display.display();
}

void resetGame() {
  scoreP1 = 0;
  scoreP2 = 0;
  isGameOver = false;
  platePosition = 22;
  remotePlatePosition = 22;
  xBall = 64;
  yBall = 32;
  lastMoveState = -1; 
  delay(500);
}





