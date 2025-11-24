/*
===============================================================================
    TEENSY PONG – 2 spillere over CAN-bus med automatisk rollefordeling
===============================================================================
... (Variabeloversikt og CAN ID-er som før) ...
NY CAN ID: 58 (Reset Spill)
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

constexpr int OLED_DC     = 6;
constexpr int OLED_CS     = 10;
constexpr int OLED_RESET  = 5;
constexpr int SCREEN_WIDTH  = 128;
constexpr int SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ------------------ CAN-konfig ------------------
constexpr int groupNumber = 6;
constexpr int idPlatePositionP1 = groupNumber + 20; // 26
constexpr int idPlatePositionP2 = groupNumber + 21; // 27
constexpr int idBallPosition    = groupNumber + 50; // 56
constexpr int idRoleClaim       = groupNumber + 10; // 16
constexpr int idGameOver        = groupNumber + 51; // 57 (Sender poeng)
constexpr int idResetGame       = groupNumber + 52; // 58 (NY: Signal for å restarte)

FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;

// ------------------ Spillvariabler ------------------
// Plate
int platePosition = 22;
int remotePlatePosition = -1;
const int plateHeight = 20;
const int plateWidth = 4;

// Ball
int xBall = 64;
int yBall = 32;
int xVelocity = 1;
int yVelocity = 1;
const int ballRadius = 3;

// Poeng
int scoreP1 = 0; // P1 = Høyre spiller
int scoreP2 = 0; // P2 = Venstre spiller

// ------------------ Spillets tilstand ------------------
bool isPlayer2 = false;
bool isPlayerAssigned = false;
bool hasClaimedRole = false;

// --- NYE VARIABLER FOR RESET ---
const int WINNING_SCORE = 5; // Spiller til 5 poeng
bool isGameOver = false;     // Styrer om spillet er over


// ============================================================================
// SETUP
// ============================================================================
void setup()
{
    Serial.begin(9600);
    delay(200);

    // Initialiser joystick
    pinMode(JOY_LEFT,  INPUT_PULLUP);
    pinMode(JOY_RIGHT, INPUT_PULLUP);
    pinMode(JOY_UP,    INPUT_PULLUP);
    pinMode(JOY_DOWN,  INPUT_PULLUP);
    pinMode(JOY_CLICK, INPUT_PULLUP); // Viktig for reset

    // Initialiser skjerm
    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println(F("ERROR: display.begin() failed."));
        while (true) delay(1000);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 28);
    display.print("Venter...");
    display.display();

    // Initialiser CAN-bus
    Can0.begin();
    Can0.setBaudRate(500000);
    Serial.println("CAN-bus startet!");
}

// ============================================================================
// Funksjons-prototyper
// ============================================================================
bool handleJoystickInput();
void handleRoleAssignment(bool moved);
void receiveCANMessages();
void updateGameLogic();
void drawDisplay();
void sendCANMessages();
void drawGameOverScreen(); // NY
void resetGame();          // NY


// ============================================================================
// HOVEDLØKKE (LOOP)
// ============================================================================
void loop()
{
    if (isGameOver) {
        // --- SPILLET ER OVER ---

        // 1. Vis "Game Over"-skjermen
        drawGameOverScreen();

        // 2. Sjekk om DENNE spilleren vil restarte (trykker klikk)
        if (digitalRead(JOY_CLICK) == LOW) {
            // Send reset-signal til den andre spilleren
            CAN_message_t resetMsg;
            resetMsg.id = idResetGame;
            resetMsg.len = 1;
            resetMsg.buf[0] = 1; // Signal om å restarte
            Can0.write(resetMsg);
            
            // Vent til knappen slippes (viktig!)
            while(digitalRead(JOY_CLICK) == LOW) delay(10);
            
            // Utfør lokal reset
            resetGame();
        }
        
        // 3. Lytt etter reset-signal fra den ANDRE spilleren
        // (Må gjøres selv om vi venter på klikk)
        receiveCANMessages(); 

    } else {
        // --- VANLIG SPIL-LØKKE ---

        // 1. Les input fra spilleren
        bool moved = handleJoystickInput();

        // 2. Håndter rollefordeling (skjer bare i starten)
        handleRoleAssignment(moved);

        // 3. Motta og behandle alle innkommende CAN-meldinger
        receiveCANMessages();

        // 4. Oppdater spill-logikk (KUN P1 gjør dette)
        if (!isPlayer2 && isPlayerAssigned) {
            updateGameLogic();
        }

        // 5. Tegn alt til skjermen
        drawDisplay();

        // 6. Send relevante CAN-meldinger
        sendCANMessages();
    }

    // 7. Kort pause for å stabilisere løkken
    delay(10); // 100 Hz
}

// ============================================================================
// HJELPEFUNKSJONER (kalt fra loop)
// ============================================================================

/*
 * Leser joystick-input og oppdaterer 'platePosition'.
 */
bool handleJoystickInput() {
    bool moved = false;
    if (digitalRead(JOY_UP) == LOW && platePosition > 0) {
        platePosition -= 2;
        moved = true;
    }
    if (digitalRead(JOY_DOWN) == LOW && platePosition < SCREEN_HEIGHT - plateHeight) {
        platePosition += 2;
        moved = true;
    }
    return moved;
}

/*
 * Håndterer logikken for å bli P1 hvis man beveger seg først.
 */
void handleRoleAssignment(bool moved) {
    if (moved && !isPlayerAssigned && !hasClaimedRole) {
        isPlayer2 = false;
        isPlayerAssigned = true;
        hasClaimedRole = true;
        Serial.println("Jeg ble Player 1!");

        CAN_message_t claimMsg;
        claimMsg.id = idRoleClaim;
        claimMsg.len = 1;
        claimMsg.buf[0] = 1;
        Can0.write(claimMsg);
    }
}

/*
 *Mottar og behandler alle CAN-meldinger i køen.
 */
void receiveCANMessages() {
    CAN_message_t rxMsg;
    while (Can0.read(rxMsg))
    {
        // 1. Rolleclaim
        if (rxMsg.id == idRoleClaim && rxMsg.len >= 1 && !isPlayerAssigned) {
            if (rxMsg.buf[0] == 1) {
                isPlayer2 = true;
                isPlayerAssigned = true;
                Serial.println("Jeg ble Player 2!");
            }
        }

        // 2. Plateposisjon fra motpart
        const int rxPlateId = isPlayer2 ? idPlatePositionP1 : idPlatePositionP2;
        if (rxMsg.id == rxPlateId && rxMsg.len >= 1) {
            remotePlatePosition = rxMsg.buf[0];
        }

        // 3. Ballposisjon (Bare P2 mottar)
        if (isPlayer2 && rxMsg.id == idBallPosition && rxMsg.len == 2) {
            xBall = SCREEN_WIDTH - rxMsg.buf[0]; // Inverter X-akse
            yBall = rxMsg.buf[1];
        }

        // 4. Motta poengsum (Bare P2 reagerer)
        if (rxMsg.id == idGameOver && isPlayer2 && rxMsg.len == 2) {
            scoreP1 = rxMsg.buf[0];
            scoreP2 = rxMsg.buf[1];
            
            // NY: Sjekk om spillet er over
            if (scoreP1 >= WINNING_SCORE || scoreP2 >= WINNING_SCORE) {
                isGameOver = true;
                // IKKE pause her, la hovedloopen håndtere "Game Over"
            }
        }

        // 5. NY: Motta Reset-signal
        if (rxMsg.id == idResetGame && rxMsg.len == 1 && rxMsg.buf[0] == 1 && isGameOver) {
            // Den andre spilleren vil restarte
            Serial.println("Mottok reset-signal!");
            resetGame();
        }
    }
}

/*
 * Oppdaterer ballens posisjon, kollisjoner og poeng.
 * DENNE FUNKSJONEN KJØRES KUN AV P1.
 */
void updateGameLogic() {
    // Kollisjon- og bevegelseslogikk (som før)
    xBall += xVelocity;
    yBall += yVelocity;

    // Kollisjon høyre plate (P1, egen)
    int plateX = SCREEN_WIDTH - plateWidth;
    if (xBall + ballRadius >= plateX &&
        yBall >= platePosition &&
        yBall <= platePosition + plateHeight &&
        xVelocity > 0) {
        xVelocity = -xVelocity;
        xBall = plateX - ballRadius;
    }

    // Kollisjon venstre plate (P2, remote)
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

    // Kollisjon vegger (topp/bunn)
    const int topWallY = 0;
    if (yBall - ballRadius <= topWallY && yVelocity < 0) {
        yVelocity = -yVelocity;
        yBall = topWallY + ballRadius;
    }
    else if (yBall + ballRadius >= SCREEN_HEIGHT && yVelocity > 0) {
        yVelocity = -yVelocity;
        yBall = SCREEN_HEIGHT - ballRadius;
    }

    // --- ENDRET: Poeng tildeles ---
    if (xBall - ballRadius > SCREEN_WIDTH || xBall + ballRadius < 0) {

        if (xBall - ballRadius > SCREEN_WIDTH) {
            scoreP2++; // P2 (venstre) scorer
        } else {
            scoreP1++; // P1 (høyre) scorer
        }

        // Send ALLTID poengsum til P2
        CAN_message_t scoreMsg;
        scoreMsg.id = idGameOver;
        scoreMsg.len = 2;
        scoreMsg.buf[0] = scoreP1;
        scoreMsg.buf[1] = scoreP2;
        Can0.write(scoreMsg);

        // NY: Sjekk om spillet er VUNNET
        if (scoreP1 >= WINNING_SCORE || scoreP2 >= WINNING_SCORE) {
            isGameOver = true;
            // IKKE reset ballen, la loopen gå til "Game Over"-tilstand
        } else {
            // Spillet er IKKE over, bare reset for neste runde
            delay(2000); // Pause for at spillerne skal se
            xBall = SCREEN_WIDTH / 2;
            yBall = SCREEN_HEIGHT / 2;
            xVelocity = -1; // Serve alltid mot P2
            yVelocity = 1;
        }
    }
}

/*
 * Tegner hele skjermbildet på nytt (scoreboard, plater, ball).
 */
void drawDisplay() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // 1. Poengtavle
    display.setTextSize(1); 
    display.setCursor(SCREEN_WIDTH/2 - 45, 0);
    if(scoreP2 < 10) display.print("0");
    display.print(scoreP2);
    display.setCursor(SCREEN_WIDTH/2 - 6, 0);
    display.print("-");
    display.setCursor(SCREEN_WIDTH/2 + 15, 0);
    if(scoreP1 < 10) display.print("0");
    display.print(scoreP1);

    // 2. Rolleindikator
    display.setTextSize(1);
    display.setCursor(0, SCREEN_HEIGHT - 8);
    if (!isPlayerAssigned) display.print("P JS 1. to be P1");
    else if (isPlayer2) display.print("P2 (V)");
    else display.print("P1 (H)");

    // 3. Plater
    if (remotePlatePosition >= 0)
        display.fillRect(0, remotePlatePosition, plateWidth, plateHeight, SSD1306_WHITE);
    display.fillRect(SCREEN_WIDTH - plateWidth, platePosition, plateWidth, plateHeight, SSD1306_WHITE);

    // 4. Ball
    display.fillCircle(xBall, yBall, ballRadius, SSD1306_WHITE);

    // 5. Vis alt
    display.display();
}

/*
 * Sender CAN-meldinger for plateposisjon og (hvis P1) ballposisjon.
 */
void sendCANMessages() {
    if (isPlayerAssigned) {
        // 1. Send EGEN plateposisjon
        CAN_message_t msgPlatePosition;
        msgPlatePosition.id = isPlayer2 ? idPlatePositionP2 : idPlatePositionP1;
        msgPlatePosition.len = 1;
        msgPlatePosition.buf[0] = platePosition;
        Can0.write(msgPlatePosition);

        // 2. Bare P1 sender ballposisjonen
        if (!isPlayer2) {
            CAN_message_t msgBall;
            msgBall.id = idBallPosition;
            msgBall.len = 2;
            msgBall.buf[0] = xBall;
            msgBall.buf[1] = yBall;
            Can0.write(msgBall);
        }
    }
}

// ============================================================================
// NYE FUNKSJONER FOR GAME OVER / RESET
// ============================================================================

/*
 * Viser vinner/taper-skjerm basert på poengsum.
 */
void drawGameOverScreen() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Sjekk hvem som vant
    bool jegVant = false;
    if (isPlayer2 && scoreP2 >= WINNING_SCORE) {
        jegVant = true;
    } else if (!isPlayer2 && scoreP1 >= WINNING_SCORE) {
        jegVant = true;
    }

    // Vis VANT eller TAPTE
    display.setTextSize(2);
    if (jegVant) {
        display.setCursor(25, 10);
        display.print("DU VANT!");
    } else {
        display.setCursor(30, 10);
        display.print("DU TAPTE");
    }

    // Vis instruksjoner for reset
    display.setTextSize(1);
    display.setCursor(20, 40);
    display.print("Trykk for nytt");
    display.setCursor(45, 50);
    display.print("spill");

    display.display();
}

/*
 * Nullstiller alle spillvariabler for en ny runde.
 */
void resetGame() {
    Serial.println("Starter nytt spill...");
    
    // Reset poeng og tilstand
    scoreP1 = 0;
    scoreP2 = 0;
    isGameOver = false;

    // Reset posisjoner
    platePosition = 22;
    remotePlatePosition = -1;
    xBall = 64;
    yBall = 32;
    xVelocity = 1; // Start mot P1
    yVelocity = 1;

    // Gi en kort pause for å unngå "flimmer" eller race conditions
    delay(500);
}
