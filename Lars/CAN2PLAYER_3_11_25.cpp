/*
===============================================================================
   TEENSY PONG – 2 spillere over CAN-bus med automatisk rollefordeling
===============================================================================

CAN id: 26 (P1 plateposisjon)
CAN id: 27 (P2 plateposisjon)
CAN id: 56 (ballposisjon)
CAN id: 16 (rolleclaim)
===============================================================================
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <FlexCAN_T4.h>

// ------------------ Hardware ------------------
constexpr int JOY_RIGHT = 17;
constexpr int JOY_LEFT  = 18;
constexpr int JOY_CLICK = 19;
constexpr int JOY_UP    = 22;
constexpr int JOY_DOWN  = 23;

constexpr int OLED_DC         = 6;
constexpr int OLED_CS         = 10;
constexpr int OLED_RESET      = 5;
constexpr int SCREEN_WIDTH    = 128;
constexpr int SCREEN_HEIGHT   = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ------------------ CAN-konfig ------------------
const int groupNumber = 6;
const int idPlatePositionP1 = groupNumber + 20; // 26
const int idPlatePositionP2 = groupNumber + 21; // 27
const int idBallPosition    = groupNumber + 50; // 56
const int idRoleClaim       = groupNumber + 10; // 16

FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;

// ------------------ Spillvariabler ------------------
int platePosition = 22;
const int plateHeight = 20;
const int plateWidth = 4;

int xBall = 64;
int yBall = 32;
int xVelocity = 1;
int yVelocity = 1;
const int ballRadius = 3;

volatile int remotePlatePosition = -1;

// ------------------ Rolledeteksjon variabler ------------------
bool isPlayer2 = false;
bool isPlayerAssigned = false;
bool hasClaimedRole = false;

// ============================================================================
// SETUP
// ============================================================================
void setup()
{
    Serial.begin(9600);
    delay(200);

    pinMode(JOY_LEFT,  INPUT_PULLUP);
    pinMode(JOY_RIGHT, INPUT_PULLUP);
    pinMode(JOY_UP,    INPUT_PULLUP);
    pinMode(JOY_DOWN,  INPUT_PULLUP);
    pinMode(JOY_CLICK, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println(F("ERROR: display.begin() failed."));
        while (true) delay(1000);
    }

    display.clearDisplay();
    display.display();

    Can0.begin();
    Can0.setBaudRate(500000);  // 500 kbit/s
    Serial.println("CAN-bus startet!");
}

// ============================================================================
// LOOP
// ============================================================================
void loop()
{
    bool moved = false;

    // --- Les joystick ---
    if (digitalRead(JOY_UP) == LOW && platePosition > 0) {
        platePosition -= 2;
        moved = true;
    }
    if (digitalRead(JOY_DOWN) == LOW && platePosition < SCREEN_HEIGHT - plateHeight) {
        platePosition += 2;
        moved = true;
    }

    // --- Rollefordeling ---
    if (moved && !isPlayerAssigned && !hasClaimedRole) {
        isPlayer2 = false;          // jeg er Player 1
        isPlayerAssigned = true;
        hasClaimedRole = true;
        Serial.println("Jeg ble Player 1!");

        CAN_message_t claimMsg;
        claimMsg.id = idRoleClaim;
        claimMsg.len = 1;
        claimMsg.buf[0] = 1;        // 1 = Player 1 claim
        Can0.write(claimMsg);
    }

    // --- Motta CAN-meldinger ---
    CAN_message_t rxMsg;
    while (Can0.read(rxMsg))
    {
        // Rolleclaim
        if (rxMsg.id == idRoleClaim && rxMsg.len >= 1 && !isPlayerAssigned) {
            if (rxMsg.buf[0] == 1) {
                isPlayer2 = true;
                isPlayerAssigned = true;
                Serial.println("Jeg ble Player 2!");
            }
        }

        // Plateposisjon fra motpart
        const int rxPlateId = isPlayer2 ? idPlatePositionP1 : idPlatePositionP2;
        if (rxMsg.id == rxPlateId && rxMsg.len >= 1) {
            remotePlatePosition = rxMsg.buf[0];
        }

        // Ballposisjon (Player 1 sender, Player 2 mottar)
        if (rxMsg.id == idBallPosition && rxMsg.len == 2) {
            xBall = rxMsg.buf[0];
            yBall = rxMsg.buf[1];
        }
    }

    // --- Oppdater ball (bare hos Player 1) ---
    if (!isPlayer2 && isPlayerAssigned) {
        xBall += xVelocity;
        yBall += yVelocity;

        // Kollisjon høyre plate (egen)
        int plateX = SCREEN_WIDTH - plateWidth;
        if (xBall + ballRadius >= plateX &&
            yBall >= platePosition &&
            yBall <= platePosition + plateHeight &&
            xVelocity > 0) {
            xVelocity = -xVelocity;
            xBall = plateX - ballRadius;
        }

        // Kollisjon venstre plate (remote)
        if (remotePlatePosition >= 0) {
            int p2X = 0;
            int rp = remotePlatePosition;
            if ((xBall - ballRadius) <= (p2X + plateWidth) &&
                yBall >= rp && yBall <= rp + plateHeight &&
                xVelocity < 0) {
                xVelocity = -xVelocity;
                xBall = p2X + plateWidth + ballRadius;
            }
        }

        // Vegger
        if (yBall <= ballRadius || yBall >= SCREEN_HEIGHT - ballRadius)
            yVelocity = -yVelocity;

        // Game over
        if (xBall - ballRadius > SCREEN_WIDTH || xBall + ballRadius < 0) {
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(15, 25);
            display.print("GAME OVER");
            display.display();
            delay(2000);
            xBall = SCREEN_WIDTH / 2;
            yBall = SCREEN_HEIGHT / 2;
            xVelocity = -1;
            yVelocity = 1;
        }
    }

    // --- Tegn alt ---
    display.clearDisplay();

    // Rolleindikator
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    if (!isPlayerAssigned) display.print("Venter...");
    else if (isPlayer2) display.print("P2");
    else display.print("P1");

    // Venstre plate (remote)
    if (remotePlatePosition >= 0)
        display.fillRect(0, remotePlatePosition, plateWidth, plateHeight, SSD1306_WHITE);

    // Høyre plate (egen)
    display.fillRect(SCREEN_WIDTH - plateWidth, platePosition, plateWidth, plateHeight, SSD1306_WHITE);

    // Ball
    display.fillCircle(xBall, yBall, ballRadius, SSD1306_WHITE);

    display.display();

    // --- CAN-sending ---
    if (isPlayerAssigned) {
        // Send egen plateposisjon
        CAN_message_t msgPlatePosition;
        msgPlatePosition.id = isPlayer2 ? idPlatePositionP2 : idPlatePositionP1;
        msgPlatePosition.len = 1;
        msgPlatePosition.buf[0] = platePosition;
        Can0.write(msgPlatePosition);

        // Bare Player 1 sender ball
        if (!isPlayer2) {
            CAN_message_t msgBall;
            msgBall.id = idBallPosition;
            msgBall.len = 2;
            msgBall.buf[0] = xBall;
            msgBall.buf[1] = yBall;
            Can0.write(msgBall);
        }
    }

    delay(15);
}
