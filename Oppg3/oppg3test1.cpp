/*
 * MAS245 - Pong Master Server
 * * FUNKSJONALITET:
 * 1. Mottar Joystick-input fra Teensy (P1) via CAN (ID 25).
 * 2. Mottar Tastatur-input fra RSP3 (P2).
 * 3. Beregner posisjon for BEGGE plater og ballen.
 * 4. Sender ALLE posisjoner tilbake til Teensy for tegning.
 */



/*
=======================================================================================
  PROSJEKT: RSP3 PONG MASTER SERVER (MAS245)
  FILNAVN:  pong_server_master.cpp
  HARDWARE: Raspberry Pi 3 (Server/P2) + Teensy 3.6 (Klient/P1)
=======================================================================================

  HVORDAN KODEN FUNGERER:
  Denne koden gjør Raspberry Pi til "Game Master". Den inneholder all spill-logikk,
  fysikkberegninger og poengsystem. Teensy fungerer nå kun som en "tynn klient" 
  som sender input og tegner det den får beskjed om.

  1. INPUT-HÅNDTERING:
     - Spiller 1 (Teensy): Sender IKKE posisjonen sin, men "ønske om bevegelse" 
       (Joystick Opp/Ned/Stille) via CAN ID 25.
     - Spiller 2 (RSP3): Styres lokalt med tastaturet ('W' og 'S').

  2. SPILLMOTOR (UPDATE LOOP - 100Hz):
     - Mottar input fra CAN (P1) og tastatur (P2).
     - Beregner ny posisjon for plater og ball basert på fart og kollisjoner.
     - Håndterer poengscoring og spillregler (Game Over).

  3. OUTPUT (DATA TIL TEENSY):
     - Sender den BEREGNEDE posisjonen til P1 tilbake til P1 (ID 26).
     - Sender posisjonen til P2 slik at P1 kan se motstanderen (ID 27).
     - Sender ballens posisjon (ID 56) og score (ID 57).

  -------------------------------------------------------------------------------------
  CAN BUS PROTOKOLL (Gruppe 6):
  
  [INPUT TIL RSP3]
  ID 25: P1 Input (0=Stille, 1=Opp, 2=Ned) -> Fra Teensy
  ID 58: Reset Signal (Hvis P1 trykker knapp) -> Fra Teensy

  [OUTPUT FRA RSP3]
  ID 26: P1 Faktisk Posisjon (Sendes tilbake til Teensy for tegning)
  ID 27: P2 Faktisk Posisjon (Sendes til Teensy for tegning av motstander)
  ID 56: Ball Posisjon (X, Y)
  ID 57: Score (P1 Score, P2 Score)
  -------------------------------------------------------------------------------------

  KOMPILERING OG KJØRING:
  1. Sørg for at CAN er oppe: sudo ip link set can0 up type can bitrate 500000
  2. Kompiler: g++ pong_server_master.cpp -o pong_server
  3. Kjør: ./pong_server

=======================================================================================
*/







#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <iostream>
#include <time.h>
#include <cstdint>

// ------------------ CAN KONFIGURASJON ------------------
const char *ifname = "can0";
int canSocketDescriptor;

const int groupNumber = 6;

// --- NY ID FOR INPUT ---
const int idJoystickP1      = groupNumber + 19; // 25 (Input fra Teensy: 1=Opp, 2=Ned, 0=Stille)

// --- ID-er FOR OUTPUT (Sendes TIL Teensy) ---
const int idPlatePositionP1 = groupNumber + 20; // 26 (RSP3 forteller hvor P1 er)
const int idPlatePositionP2 = groupNumber + 21; // 27 (RSP3 forteller hvor P2 er)
const int idBallPosition    = groupNumber + 50; // 56
const int idGameOver        = groupNumber + 51; // 57
const int idResetGame       = groupNumber + 52; // 58

// ------------------ SPILL-VARIABLER ------------------
const int SCREEN_WIDTH  = 128;
const int SCREEN_HEIGHT = 64;

// Plater
int platePosP1 = 22; 
int platePosP2 = 22;
const int plateHeight = 20;
const int plateWidth = 4;
const int plateSpeed = 3; // Hvor fort platene beveger seg per 'tick'

// Input lagring
int p1MoveState = 0; // 0=Stille, 1=Opp, 2=Ned

// Ball
int xBall = 64;
int yBall = 32;
int xVelocity = 1;
int yVelocity = 1;
const int ballRadius = 3;

// Score
int scoreP1 = 0;
int scoreP2 = 0;
const int WINNING_SCORE = 5;
bool isGameOver = false;

const int taskSleepTimeUs = 10000; // 10ms = 100Hz

// ============================================================================
// TASTATUR (P2 Input)
// ============================================================================
void setNonBlockingKeyboard(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) & ~O_NONBLOCK);
    }
}

void handleKeyboardInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        // P2 Kontroll (Direkte oppdatering av posisjon er ok for P2 lokalt)
        if (c == 'w' || c == 'W') {
            if (platePosP2 > 0) platePosP2 -= plateSpeed;
        }
        if (c == 's' || c == 'S') {
            if (platePosP2 < SCREEN_HEIGHT - plateHeight) platePosP2 += plateSpeed;
        }
        // Manuell Reset
        if ((c == 'r' || c == 'R') && isGameOver) {
           resetGame();  // Håndteres i main via flaggreset
        }
    }
}

// ============================================================================
// CAN SYSTEM
// ============================================================================
bool createCanSocket(int& socketDescriptor) {
    struct sockaddr_can addr;
    struct ifreq ifr;
    if ((socketDescriptor = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) return false;
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(socketDescriptor, SIOCGIFINDEX, &ifr) < 0) return false;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socketDescriptor, (struct sockaddr *)&addr, sizeof(addr)) < 0) return false;
    return true;
}

void sendCanMessage(int socketDescriptor, int id, uint8_t* data, int len) {
    struct can_frame frame;
    frame.can_id = id;
    frame.can_dlc = len;
    memcpy(frame.data, data, len);
    write(socketDescriptor, &frame, sizeof(struct can_frame));
}

// ============================================================================
// LOGIKK
// ============================================================================
void resetGame() {
    std::cout << "\n--- RESET ---" << std::endl;
    scoreP1 = 0;
    scoreP2 = 0;
    isGameOver = false;
    
    // Tilbakestill fysikk
    xBall = SCREEN_WIDTH / 2;
    yBall = SCREEN_HEIGHT / 2;
    xVelocity = -1; 
    yVelocity = 1;
    platePosP1 = 22;
    platePosP2 = 22;

    // --- NYTT: Send beskjed til Teensy om at spillet er reset ---
    // Dette vekker Teensy fra "Game Over"-skjermen
    uint8_t resetData[1] = {1};
    sendCanMessage(canSocketDescriptor, idResetGame, resetData, 1);
}
void updatePhysics() {
    if (isGameOver) return;

    // 1. Oppdater P1 Posisjon basert på mottatt input
    // Protokoll fra Teensy: 1 = OPP, 2 = NED, 0 = STILLE
    if (p1MoveState == 1 && platePosP1 > 0) {
        platePosP1 -= plateSpeed;
    } else if (p1MoveState == 2 && platePosP1 < SCREEN_HEIGHT - plateHeight) {
        platePosP1 += plateSpeed;
    }

    // 2. Oppdater Ball
    xBall += xVelocity;
    yBall += yVelocity;

    // Kollisjon P1 (Høyre)
    int p1X = SCREEN_WIDTH - plateWidth;
    if (xBall + ballRadius >= p1X &&
        yBall >= platePosP1 && yBall <= platePosP1 + plateHeight && xVelocity > 0) {
        xVelocity = -xVelocity;
        xBall = p1X - ballRadius;
    }

    // Kollisjon P2 (Venstre)
    int p2X = 0 + plateWidth; 
    if (xBall - ballRadius <= p2X &&
        yBall >= platePosP2 && yBall <= platePosP2 + plateHeight && xVelocity < 0) {
        xVelocity = -xVelocity;
        xBall = p2X + ballRadius;
    }

    // Vegger
    if (yBall - ballRadius <= 0 && yVelocity < 0) {
        yVelocity = -yVelocity;
        yBall = ballRadius;
    } else if (yBall + ballRadius >= SCREEN_HEIGHT && yVelocity > 0) {
        yVelocity = -yVelocity;
        yBall = SCREEN_HEIGHT - ballRadius;
    }

    // Score
    bool scored = false;
    if (xBall > SCREEN_WIDTH) { scoreP2++; scored = true; }
    else if (xBall < 0)       { scoreP1++; scored = true; }

    if (scored) {
        uint8_t scoreData[2] = {(uint8_t)scoreP1, (uint8_t)scoreP2};
        sendCanMessage(canSocketDescriptor, idGameOver, scoreData, 2);
        std::cout << "\rScore: " << scoreP1 << " - " << scoreP2 << std::flush;

        if (scoreP1 >= WINNING_SCORE || scoreP2 >= WINNING_SCORE) {
            isGameOver = true;
        } else {
            usleep(1000000);
            xBall = SCREEN_WIDTH / 2;
            yBall = SCREEN_HEIGHT / 2;
            xVelocity = -xVelocity; 
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
    if (!createCanSocket(canSocketDescriptor)) return 1;
    setNonBlockingKeyboard(true);

    std::cout << "Master Server Started." << std::endl;

    while (true) {
        // --- 1. LES INPUT (CAN + Tastatur) ---
        struct can_frame rxFrame;
        while (recv(canSocketDescriptor, &rxFrame, sizeof(struct can_frame), MSG_DONTWAIT) > 0) {
            
            // Mottar input-kommando fra Teensy (ID 25)
            if (rxFrame.can_id == idJoystickP1) {
                p1MoveState = rxFrame.data[0]; // 1=Opp, 2=Ned, 0=Stille
            }
            
            // Reset-signal
            else if (rxFrame.can_id == idResetGame && rxFrame.data[0] == 1) {
                resetGame();
            }
        }
        
        handleKeyboardInput(); // P2 Input

        // --- 2. BEREGN FYSIKK ---
        updatePhysics();

        // --- 3. SEND DATA TIL TEENSY ---
        if (!isGameOver) {
            // A. Send Ball
            uint8_t ballData[2] = {(uint8_t)xBall, (uint8_t)yBall};
            sendCanMessage(canSocketDescriptor, idBallPosition, ballData, 2);

            // B. Send P1 Posisjon (Så Teensy vet hvor den selv er!)
            // Dette er nytt: Vi sender ID 26 tilbake til eieren av ID 26
            uint8_t p1Data[1] = {(uint8_t)platePosP1};
            sendCanMessage(canSocketDescriptor, idPlatePositionP1, p1Data, 1);

            // C. Send P2 Posisjon (Så Teensy ser motstander)
            uint8_t p2Data[1] = {(uint8_t)platePosP2};
            sendCanMessage(canSocketDescriptor, idPlatePositionP2, p2Data, 1);
        }

        usleep(taskSleepTimeUs);
    }

    setNonBlockingKeyboard(false);
    close(canSocketDescriptor);
    return 0;
}