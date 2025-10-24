#include <FlexCAN_T4.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/TomThumb.h> 

// OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC     6
#define OLED_CS     10
#define OLED_RESET  5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// CAN setup
FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;
static CAN_message_t txmsg;
static volatile uint32_t tx_count = 0;

// Last-received info for OLED
static CAN_message_t last_rx;
static volatile bool have_rx = false;

// >>> ADD: counters
static volatile uint32_t rx_count = 0;
static volatile uint32_t echo_count = 0;

// --------------- CAN Receive Callbacks ---------------
void onCan0Receive(const CAN_message_t &msg) {
  last_rx = msg;
  have_rx = true;

  // >>> CHANGE: don't write() from ISR; echo in loop()
  // CAN_message_t echo = msg; Can0.write(echo);
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

void setup() {
  pinMode(13, OUTPUT);

  Serial.begin(115200);
  delay(300);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 init failed (check wiring).");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Teensy 3.6 CAN demo");
    display.println("FlexCAN_T4 + OLED");
    display.display();
  }

  // ----- CAN init -----
  const uint32_t BITRATE = 500000; // 500 kbit/s

  Can0.begin();
  Can0.setBaudRate(BITRATE);
  Can0.setMaxMB(16);
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(onCan0Receive);
  Can0.mailboxStatus();

  // Prebuild a TX frame
  txmsg.id  = 0x123;   // standard 11-bit ID
  txmsg.len = 8;       // classic CAN payload
  txmsg.flags.extended = 0;
  txmsg.flags.remote   = 0;
  for (uint8_t i = 0; i < txmsg.len; i++) txmsg.buf[i] = 0;

  Serial.println("CAN started @ 500 kbit/s.");
  Serial.println("Sends 0x123 every second. Echoes any RX back on same CAN.");
}

elapsedMillis tick;

void loop() {
  Can0.events();

  // >>> TX once per second, include TX counter in payload
  if (tick >= 1000) {
    tick = 0;

    // Put tx_count into first 4 bytes (big-endian)
    txmsg.buf[0] = (tx_count >> 24) & 0xFF;
    txmsg.buf[1] = (tx_count >> 16) & 0xFF;
    txmsg.buf[2] = (tx_count >>  8) & 0xFF;
    txmsg.buf[3] = (tx_count >>  0) & 0xFF;
    txmsg.buf[4] = 0xAA; // markers so you can recognize the frame
    txmsg.buf[5] = 0x55;
    txmsg.buf[6] = 0xDE;
    txmsg.buf[7] = 0xAD;

    bool ok0 = Can0.write(txmsg);
    digitalWrite(13, !digitalRead(13)); // blink LED on each TX

    Serial.print("[TX#"); Serial.print(tx_count); Serial.print("] ");
    Serial.print("ID 0x"); Serial.print(txmsg.id, HEX);
    Serial.print(" DLC "); Serial.print(txmsg.len);
    Serial.print(" DATA "); Serial.println(bytesToHexString(txmsg));

    if (!ok0) {
      Serial.println("Warning: TX failed (check bus, bitrate, termination).");
    }

    tx_count++; // >>> increment after send attempt
  }

  // >>> Handle RX (and echo) in thread context
  if (have_rx) {
    noInterrupts();              // brief critical section
    CAN_message_t m = last_rx;
    have_rx = false;
    interrupts();

    rx_count++;

    // Echo back what we received
    Can0.write(m);
    echo_count++;

    String data = bytesToHexString(m);

    Serial.print("[RX#"); Serial.print(rx_count); Serial.print("] ");
    Serial.print("ID 0x"); Serial.print(m.id, HEX);
    Serial.print(m.flags.extended ? " (EXT)" : " (STD)");
    Serial.print(" DLC "); Serial.print(m.len);
    Serial.print(" DATA "); Serial.println(data);

    Serial.print("[ECHO#"); Serial.print(echo_count); Serial.print("] ");
    Serial.print("ID 0x"); Serial.print(m.id, HEX);
    Serial.print(" DLC "); Serial.print(m.len);
    Serial.print(" DATA "); Serial.println(data);

    // >>> OLED with counters
    if (display.width() > 0) {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      oledPrintLine(0, 0,  "Teensy 3.6 CAN demo");
      oledPrintLine(0, 12, "RX ID: 0x" + String(m.id, HEX));
      oledPrintLine(0, 24, "DLC: " + String(m.len));
      oledPrintLine(0, 36, "DATA:");
      oledPrintLine(0, 48, data);
      // extra line for counters (trim if you prefer)
      // Small font fits; otherwise drop one of these:
      // (If it clips, remove or reformat.)
      // oledPrintLine(0, 58, "TX:" + String(tx_count) + " RX:" + String(rx_count));
      display.display();
    }
  }

  // >>> Optional: periodic summary
  static elapsedMillis summaryTick;
  if (summaryTick >= 10000) {
    summaryTick = 0;
    Serial.print("[SUM] TX="); Serial.print(tx_count);
    Serial.print(" RX="); Serial.print(rx_count);
    Serial.print(" ECHO="); Serial.println(echo_count);
  }


//display kommer under her:

display.clearDisplay();


// Kant rundt skjermen
display.drawRoundRect(0, 0, 128, 64, 10, SSD1306_WHITE);
  
//HalvSirkelen under teksta
display.drawCircle(64, -282, 300, SSD1306_WHITE);

// Teksten
display.setFont();

display.setCursor(14, 5); // når man bruker 64 så starter den å skrive fra klink midten. Må kompansere
display.print(F("MAS245 - Gruppe 6")); //print(F()) sparer RAM med å sette strings i flash


display.setFont(&TomThumb); //dette vil gi mindre tekst

display.setCursor(5, 28); // 
display.print(F("CAN - Statistikk:"));

display.setCursor(5, 38); // 
display.print(F("Antall mottat:")); Serial.print(rx_count);
  
display.setCursor(5, 48); // 
display.print(F("Mottok sist ID:"));  Serial.print("0x"); Serial.print(m.id, HEX);

display.setCursor(5, 58); // 
display.print(F("IMU Maaling z:"));  



}

// kode dump
