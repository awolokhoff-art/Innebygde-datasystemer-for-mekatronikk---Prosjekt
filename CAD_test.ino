/* Teensy 3.6 Dual CAN + OLED demo (modernized)
 *  - Uses FlexCAN_T4 (not the old FlexCAN)
 *  - Sends 1 frame/sec on CAN0
 *  - Receives and echoes any frame on CAN0 (and CAN1 if enabled)
 *  - Displays last RX on SSD1306 128x64 SPI OLED
 *
 *  Wiring (CAN):
 *    Proper 120 Ω termination on BOTH ends (H<->L).
 *    Share GND between Teensy and other node (e.g., PCAN DB9 pin 3).
 *
 *  OLED (SPI, pins as per your earlier sketch):
 *    DC = 6, CS = 10, RST = 5
 */

#include <FlexCAN_T4.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Optional font (uncomment if you want to use it)
// #include <Fonts/FreeMono9pt7b.h>

// -------------------- OLED Setup --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC     6
#define OLED_CS     10
#define OLED_RESET  5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// -------------------- Joystick pins (not required, but kept) --------------------
const int JOY_LEFT  = 18;
const int JOY_RIGHT = 17;
const int JOY_CLICK = 19;
const int JOY_UP    = 22;
const int JOY_DOWN  = 23;

// -------------------- CAN Setup --------------------
// Teensy 3.6 supports two classic CAN controllers: CAN0 and CAN1.
// CAN FD is NOT supported on 3.6, so payload is max 8 bytes.
FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;
// If you want to also use CAN1, set this to true and wire the second channel.
const bool USE_CAN1 = false;
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;

static CAN_message_t txmsg;
static volatile uint32_t tx_count = 0;

// Last-received info for OLED
static CAN_message_t last_rx;
static volatile bool have_rx = false;

// --------------- CAN Receive Callbacks ---------------
void onCan0Receive(const CAN_message_t &msg) {
  last_rx = msg;
  have_rx = true;

  // Echo back the same message on CAN0
  CAN_message_t echo = msg;
  Can0.write(echo);
}

void onCan1Receive(const CAN_message_t &msg) {
  last_rx = msg;
  have_rx = true;

  // Echo back the same message on CAN1
  CAN_message_t echo = msg;
  if (USE_CAN1) Can1.write(echo);
}

// -------------------- Helpers --------------------
void oledPrintLine(int16_t x, int16_t y, const String &s) {
  display.setCursor(x, y);
  display.print(s);
}

String bytesToHexString(const CAN_message_t &m) {
  String s;
  for (uint8_t i = 0; i < m.len; i++) {
    uint8_t b = m.buf[i];
    if (b < 0x10) s += '0';
    s += String(b, HEX);
    if (i < m.len - 1) s += ' ';
  }
  s.toUpperCase();
  return s;
}

// -------------------- Arduino --------------------
void setup() {
  pinMode(13, OUTPUT); // onboard LED

  // Joystick pins (kept from your sketch; not required for CAN)
  pinMode(JOY_LEFT,  INPUT_PULLUP);
  pinMode(JOY_RIGHT, INPUT_PULLUP);
  pinMode(JOY_CLICK, INPUT_PULLUP);
  pinMode(JOY_UP,    INPUT_PULLUP);
  pinMode(JOY_DOWN,  INPUT_PULLUP);

  Serial.begin(115200);
  delay(300);

  // ----- OLED init -----
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    // If OLED init fails, keep going but note it on Serial
    Serial.println("SSD1306 init failed (check wiring).");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    // display.setFont(&FreeMono9pt7b); // Optional: nicer but larger font
    display.setCursor(0, 0);
    display.println("Teensy 3.6 CAN demo");
    display.println("FlexCAN_T4 + OLED");
    display.display();
  }

  // ----- CAN init -----
  // Use classic CAN (max 8-byte payload). Match bitrate with PCAN-View or other node.
  const uint32_t BITRATE = 500000; // 500 kbit/s is common

  Can0.begin();
  Can0.setBaudRate(BITRATE);
  Can0.setMaxMB(16);
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(onCan0Receive);
  Can0.mailboxStatus();

  if (USE_CAN1) {
    Can1.begin();
    Can1.setBaudRate(BITRATE);
    Can1.setMaxMB(16);
    Can1.enableFIFO();
    Can1.enableFIFOInterrupt();
    Can1.onReceive(onCan1Receive);
    Can1.mailboxStatus();
  }

  // Prebuild a TX frame
  txmsg.id  = 0x123;   // standard 11-bit ID; set txmsg.flags.extended = 1 for 29-bit
  txmsg.len = 8;       // Teensy 3.6 (classic CAN) supports 0..8
  txmsg.flags.extended = 0;
  txmsg.flags.remote   = 0;

  Serial.println("CAN started @ 500 kbit/s.");
  Serial.println("Sends 0x123 every second. Echoes any RX back on same CAN.");
}

elapsedMillis tick;

void loop() {
  // Periodic transmit (once per second)
  if (tick >= 1000) {
    tick = 0;

    // Example payload
    txmsg.buf[0] = 0xDE; txmsg.buf[1] = 0xAD; txmsg.buf[2] = 0xBE; txmsg.buf[3] = 0xEF;
    txmsg.buf[4] = (tx_count >> 24) & 0xFF;
    txmsg.buf[5] = (tx_count >> 16) & 0xFF;
    txmsg.buf[6] = (tx_count >>  8) & 0xFF;
    txmsg.buf[7] = (tx_count      ) & 0xFF;

    bool ok0 = Can0.write(txmsg);
    bool ok1 = USE_CAN1 ? Can1.write(txmsg) : true; // if not using CAN1, treat as OK

    tx_count++;
    digitalWrite(13, !digitalRead(13)); // blink LED on each TX

    Serial.print("TX 0x"); Serial.print(txmsg.id, HEX);
    Serial.print(" DLC "); Serial.print(txmsg.len);
    Serial.print(" Data "); Serial.println(bytesToHexString(txmsg));

    if (!ok0 || !ok1) {
      Serial.println("Warning: TX failed (check bus, bitrate, termination).");
    }
  }

  // Service CAN event queues for callbacks
  Can0.events();
  if (USE_CAN1) Can1.events();

  // If we received something, show on OLED + Serial
  if (have_rx) {
    have_rx = false;

    String data = bytesToHexString(last_rx);

    Serial.print("RX 0x"); Serial.print(last_rx.id, HEX);
    Serial.print(" DLC "); Serial.print(last_rx.len);
    Serial.print(" Data "); Serial.println(data);

    if (display.width() > 0) {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      oledPrintLine(0, 0,  "Teensy 3.6 CAN demo");
      oledPrintLine(0, 12, "RX ID: 0x" + String(last_rx.id, HEX));
      oledPrintLine(0, 24, "DLC: " + String(last_rx.len));
      oledPrintLine(0, 36, "DATA:");
      oledPrintLine(0, 48, data);
      display.display();
    }
  }
}
