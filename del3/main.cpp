/*
 * MAS245 - Pong Master Server
 * I denne delen så skal en RSP3 ta og hoste et pingpong spill.
 * Den skal kommunisere med en Teensy 3.6 som skal tegne selve spillet og sende ut sin joystick posisjon til RSP3
 * Denne koden skal da kunne:
 * 1. Mottar Joystick-input fra Teensy (P1) via CAN (ID 25).
 * 2. Mottar Tastatur-input fra RSP3 (P2).
 * 3. Beregner posisjon for BEGGE plater og ballen.
 * 4. Sender ALLE posisjoner tilbake til Teensy for tegning.
 */

/*
  CAN BUS PROTOKOLL (Gruppe 6):

  [INPUT TIL RSP3]
  ID 25: P1 Input (0=Stille, 1=Opp, 2=Ned) -> Fra Teensy
  ID 58: Reset Signal (Hvis P1 trykker knapp) -> Fra Teensy

  [OUTPUT FRA RSP3]
  ID 26: P1 Faktisk Posisjon (Sendes tilbake til Teensy for tegning)
  ID 27: P2 Faktisk Posisjon (Sendes til Teensy for tegning av motstander)
  ID 56: Ball Posisjon (X, Y)
  ID 57: Score (P1 Score, P2 Score)
  ID 59: Reset signal (en melding som sier til teensyen at alt skal resetes)
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
#include <algorithm>
#include <chrono>



// CAN KONFIGURASJON
const char *ifname = "can0";
int canSocketDescriptor;

const int groupNumber = 6;

// ID FOR INPUT
const int idJoystickP1        = groupNumber + 19; // 25 (Input fra Teensy: 1=Opp, 2=Ned, 0=Stille)
const int idResetRequest      = groupNumber + 52; // 58 (Teensy → RPi Teensyen som ber RPi om å resette)

// ID-er for Outputs (Sendes TIL Teensy)
const int idPlatePositionP1   = groupNumber + 20; // 26 (RSP3 forteller hvor P1 er)
const int idPlatePositionP2   = groupNumber + 21; // 27 (RSP3 forteller hvor P2 er)
const int idBallPosition      = groupNumber + 50; // 56 (Ball posisjonen)
const int idScore             = groupNumber + 51; // 57 (Scoren)
const int idResetAcknowledge  = groupNumber + 53; // 59 (RPi → Teensy RPi som sender en melding til teensyen om at spillet faktisk blir resatt)

// Spill-variabler
// Skjerm
const int SCREEN_WIDTH  = 128;
const int SCREEN_HEIGHT = 64;

// Plater
int platePosP1 = 22;
int platePosP2 = 22;
const int plateHeight = 20;
const int plateWidth = 4;
const int plateSpeedP1 = 4; // Hvor fort platen for P1 beveger seg per 'tick'
const int plateSpeedP2 = 3; // Hvor fort platen for P2 beveger seg per 'tick'

// Input lagring
int p1MoveState = 0; // 0=Stille, 1=Opp, 2=Ned
int p2MoveState = 0; // 0=Stille, 1=Opp, 2=Ned
int p2HoldCounter = 0;     // teller hvor mange 'tick' den skal gå mens den holdes nede
const int holdThreshold = 4; // beveger seg med 4 'ticks' når knappen blir holdt nede
bool wPressed = false;
bool sPressed = false;


// Ball
int xBall = 64;  // ballens startposisjon i x retning
int yBall = 32;  // ballens startposisjon i y retning
int ballXVelocity = 1; // ballens hastighet i x retning
int ballYVelocity = 1; // ballens hastighet i y retning
const int ballRadius = 3; // størrelsen på ballen

// Score
int scoreP1 = 0;
int scoreP2 = 0;
const int WINNING_SCORE = 5;
bool isGameOver = false;

// Pause i koden
const int taskSleepTimeUs = 10000; // 10ms = 100Hz
bool isPaused = false;
std::chrono::steady_clock::time_point pauseEndTime;


// Tastatur (P2 Input)
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

// CAN System
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


// Logikk

void updatePhysics() {
    if (isGameOver) return;

    if (isPaused){
        if (std::chrono::steady_clock::now() >= pauseEndTime) {
            isPaused = false; // setter liten pustepause etter at noen har skåret et poeng
        } else {
            return;
        }
    }
    // P1 movement
    if (p1MoveState == 1) platePosP1 = std::max(0, platePosP1 - plateSpeedP1);
    if (p1MoveState == 2) platePosP1 = std::min(SCREEN_HEIGHT - plateHeight, platePosP1 + plateSpeedP1);

    // P2 movement
    if (p2MoveState != 0) {
        p2HoldCounter++;
        if (p2HoldCounter == 1 || p2HoldCounter > holdThreshold) {
            if (p2MoveState == 1) platePosP2 = std::max(0, platePosP2 - plateSpeedP2);
            if (p2MoveState == 2) platePosP2 = std::min(SCREEN_HEIGHT - plateHeight, platePosP2 + plateSpeedP2);
            if (p2HoldCounter > holdThreshold) p2HoldCounter = holdThreshold - 1;
        }
    } else {
        p2HoldCounter = 0;
    }

    // Oppdater Ball
    xBall += ballXVelocity;
    yBall += ballYVelocity;

    // Kollisjon P1 (Høyre)
    int p1X = SCREEN_WIDTH - plateWidth;
    if (xBall + ballRadius >= p1X &&
        yBall >= platePosP1 && yBall <= platePosP1 + plateHeight && ballXVelocity > 0) {
        ballXVelocity = -ballXVelocity;
        xBall = p1X - ballRadius;
    }

    // Kollisjon P2 (Venstre)
    int p2X = 0 + plateWidth;
    if (xBall - ballRadius <= p2X &&
        yBall >= platePosP2 && yBall <= platePosP2 + plateHeight && ballXVelocity < 0) {
        ballXVelocity = -ballXVelocity;
        xBall = p2X + ballRadius;
    }

    // Vegger
    if (yBall - ballRadius <= 0 && ballYVelocity < 0) {
        ballYVelocity = -ballYVelocity;
        yBall = ballRadius;
    } else if (yBall + ballRadius >= SCREEN_HEIGHT && ballYVelocity > 0) {
        ballYVelocity = -ballYVelocity;
        yBall = SCREEN_HEIGHT - ballRadius;
    }

    // Score
    bool scored = false;
    if (xBall > SCREEN_WIDTH) { scoreP2++; scored = true; }
    else if (xBall < 0)       { scoreP1++; scored = true; }

    if (scored) {
        uint8_t scoreData[2] = {(uint8_t)scoreP1, (uint8_t)scoreP2};
        sendCanMessage(canSocketDescriptor, idScore, scoreData, 2);
        std::cout << "\rScore: " << scoreP1 << " - " << scoreP2 << std::flush;

        if (scoreP1 >= WINNING_SCORE || scoreP2 >= WINNING_SCORE) {
            isGameOver = true;
        } else {
            isPaused = true;
            pauseEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            xBall = SCREEN_WIDTH / 2;
            yBall = SCREEN_HEIGHT / 2;
            ballXVelocity = (scoreP1 > scoreP2) ? -1 : 1;
            ballYVelocity = 1;
        }
    }
}

void resetGame() {
    std::cout << "\n--- RESET ---" << std::endl;
    scoreP1 = 0;
    scoreP2 = 0;
    isGameOver = false;

    xBall = SCREEN_WIDTH / 2;
    yBall = SCREEN_HEIGHT / 2;
    ballXVelocity = 1;
    ballYVelocity = 1;
    platePosP1 = 22;
    platePosP2 = 22;

    p2HoldCounter = 0;
    p2MoveState = 0;
    wPressed = sPressed = false;

    // Send beskjed til Teensy om at spillet er reset
    // Dette vekker Teensy fra "Game Over"-skjermen
    uint8_t resetData[1] = {1};
    sendCanMessage(canSocketDescriptor, idResetAcknowledge, resetData, 1);

}

void handleKeyboardInput() {
    char c;

    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == 'w' || c == 'W') wPressed = true;
        if (c == 's' || c == 'S') sPressed = true;
        if (c == 'r' || c == 'R') resetGame();
    }

    //Sjekker om knappene blir holdt eller bare trykket
    //Lagt til pga oppdaget feil mens spillet ble kjørt
    if (!wPressed && !sPressed) p2MoveState = 0;
    else if (wPressed) p2MoveState = 1;
    else if (sPressed) p2MoveState = 2;

    wPressed = false;
    sPressed = false;
}

// Main
int main() {
    if (!createCanSocket(canSocketDescriptor)) return 1;
    setNonBlockingKeyboard(true);

    std::cout << "Master Server Started." << std::endl;

    while (true) {
        // Leser input  fra CAN og tastatur
        struct can_frame rxFrame;
        p1MoveState = 0;
        while (recv(canSocketDescriptor, &rxFrame, sizeof(struct can_frame), MSG_DONTWAIT) > 0) {

            // Mottar input-kommando fra Teensy (ID 25)
            if (rxFrame.can_id == idJoystickP1) {
                p1MoveState = rxFrame.data[0]; // 1=Opp, 2=Ned, 0=Stille
            }

            // Reset-signal
            else if (rxFrame.can_id == idResetRequest && rxFrame.data[0] == 1) {
                std::cout << "Received reset request from Teensy\n";
                resetGame();
                continue;
            }
        }

        handleKeyboardInput(); // P2 Input

        // Bergn fysikken
        updatePhysics();


        // Sender data til Teensy
        if (!isGameOver) {
            //Sender ball
            uint8_t ballData[2] = {(uint8_t)xBall, (uint8_t)yBall};
            sendCanMessage(canSocketDescriptor, idBallPosition, ballData, 2);

            // Sender P1 Posisjon (Så Teensy vet hvor den selv er!)
            uint8_t p1Data[1] = {(uint8_t)platePosP1};
            sendCanMessage(canSocketDescriptor, idPlatePositionP1, p1Data, 1);

            // Sender P2 Posisjon (Så Teensy ser motstander)
            uint8_t p2Data[1] = {(uint8_t)platePosP2};
            sendCanMessage(canSocketDescriptor, idPlatePositionP2, p2Data, 1);
        }

        usleep(taskSleepTimeUs);
    }

    setNonBlockingKeyboard(false);
    close(canSocketDescriptor);
    return 0;
}
