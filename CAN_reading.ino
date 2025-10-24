#include <FlexCAN_T4.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Setup, definerer pins til variabelnavn
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC     6
#define OLED_CS     10
#define OLED_RESET  5

//lager display object, og forteller hvordan OLED skjermen er koblet. når dispaly blir brukt snakker den med OLED skjermen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS); 

// CAN setup
FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0; // lager et CAN interface object, med navn Can0, som kommuniserer med Teensy sin CAN0 hardware, med en 256 byte reciver buffer og en 16 byte transmitter buffer
static CAN_message_t txmsg; // en struct som brukes til meldingene som blir sendt
static volatile uint32_t tx_count = 0; // en struct som teller hvor mange meldinger som har blit sendt

// Last-received info for OLED
static CAN_message_t last_rx; // en struct som inneholder den siste CAN meldingen
static volatile bool have_rx = false; // en bool som forteller som en ny CAN melding har blitt sendt

// Counters
static volatile uint32_t rx_count = 0; // en struct som teller hvor mange meldinger som blir motatt
static volatile uint32_t echo_count = 0; // en struct som teller hvor mange meldinger teensyen har echoed tilbake på CAN busen

// --------------- CAN Receive Callbacks ---------------
void onCan0Receive(const CAN_message_t &msg) // funksjon som kjøres hver gang en CAN meldigner blir mottatt, lagrer meldingen i last_rx og setter have_rx til true, slik at hovedløkken vet at ny data er tilgjengelig
{
  last_rx = msg;
  have_rx = true;
}

// -------------------- Helpers --------------------
void oledPrintLine(int16_t x, int16_t y, const String &s) //funskjon for å skrive teskt på OLEd-skjermen ved posisjon (x,y)
{
  display.setCursor(x, y);
  display.print(s);
}

String bytesToHexString(const CAN_message_t &m) // tar en CAN meldinger som ankommer i bytes og konverter det til en hex string
{
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

void setup() 
{
  pinMode(13, OUTPUT);  // setter pin 13 på teensy som en Output

  Serial.begin(115200); // setter kommunikasjonshastigheten mellom Teensy og PC til 115200 but per sekunder, som er en standar UART overførsingshastighet, kan også endres til [9600, 19200, 38400, 57600] hvis tregere respons er ønsket

  delay(300);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) // initialiserer OLED skjermen
  {
    Serial.println("SSD1306 init failed (check wiring)."); // Error melding hvis initialisering ikke funket
  } else // setter opp OLED skjermen, med en demo tekst opp i venstre hjørne
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Teensy 3.6 CAN demo"); // printer en ny linje med teskten "Teensy 3.6 CAN demo" og flytter cursor ned til neste linje
    display.println("FlexCAN_T4 + OLED"); // printer en ny linje med teskten "FlexCAN_T4 + OLED"og flytter cursor ned til neste linje
    display.display();
  }

  // ----- CAN init -----
  const uint32_t BITRATE = 500000; //Setter CAN-bus hastigheten til 500 kbit/s

// initialisere og konfigurerer CAN på Teensy
  Can0.begin(); // initialiserer CAN0-kontrolleren på Teensy
  Can0.setBaudRate(BITRATE); // setter CAN hastigheten
  Can0.setMaxMB(16); // Setter mac meldingsbuffer for Can-kommunikasjon
  Can0.enableFIFO(); // Skrur på FIFO (First In,First Out) modus for å mota meldinger
  Can0.enableFIFOInterrupt(); // Skrur på interrupt når en ny melding mottas av FIFO
  Can0.onReceive(onCan0Receive);// Registrerer callback-funksjonen som håndterer mottatte meldinger
  Can0.mailboxStatus();// Skriver status for mailbokser/FIFO til Serial Monitor (for feilsøking)

  // Prebuild a TX frame
  txmsg.id  = 0x123;   // Meldings_ID
  txmsg.len = 8;       // Hvor mange data bytese som blir sendt
  txmsg.flags.extended = 0; //0=standard (11-bit), 1=utvidet (29-bit)
  txmsg.flags.remote   = 0; // 0=dataramme/pakke, 1=remote (RTR) forespørsel
  for (uint8_t i = 0; i < txmsg.len; i++) txmsg.buf[i] = 0; // setter alle 8 databyte til 0 slik at første melding sendes som 0 0 0 0 0 0 0 0

  Serial.println("CAN started @ 500 kbit/s."); 
  Serial.println("Sends 0x123 every second. Echoes any RX back on same CAN."); 

elapsedMillis tick; // Timer som teller millisekunder automatisk, brukes for å sende melding hvert sekund

void loop() {
  Can0.events(); // Behandler CAN-hendelser (mottak/sending)

  // >>> TX once per second, include TX counter in payload
  if (tick >= 1000) {
    tick = 0;

    // Put tx_count into first 4 bytes (big-endian)
    // Bruker de 4 siste bytene som faste markører (AA 55 DE AD) for å kjenne igjen meldingen

    txmsg.buf[0] = (tx_count >> 24) & 0xFF;
    txmsg.buf[1] = (tx_count >> 16) & 0xFF;
    txmsg.buf[2] = (tx_count >>  8) & 0xFF;
    txmsg.buf[3] = (tx_count >>  0) & 0xFF;
    txmsg.buf[4] = 0xAA; // markers so you can recognize the frame
    txmsg.buf[5] = 0x55;
    txmsg.buf[6] = 0xDE;
    txmsg.buf[7] = 0xAD;

    bool ok0 = Can0.write(txmsg);  // prøver å sende meldingen på CAN-bussen
    digitalWrite(13, !digitalRead(13)); // blink LED on each TX

   // Skriver ut den sendte meldingen i Serial Monitor
    Serial.print("[TX#"); Serial.print(tx_count); Serial.print("] ");
    Serial.print("ID 0x"); Serial.print(txmsg.id, HEX);
    Serial.print(" DLC "); Serial.print(txmsg.len);
    Serial.print(" DATA "); Serial.println(bytesToHexString(txmsg));

    // Skriver advarsel hvis CAN-meldingen ikke ble sendt
    if (!ok0) {
      Serial.println("Warning: TX failed (check bus, bitrate, termination).");
    }

    tx_count++; // øker tx_count for hver melding som blir sendt
  }

  // >>> Handle RX (and echo) in thread context
  if (have_rx) // Hvis en melding har blitt motatt så skal meldingen tolkes 
  {
    noInterrupts(); // Slår av alle maskinvare-avbrudd midlertidig for å beskytte kritisk kode
    CAN_message_t m = last_rx; // kopierer siste mottatte melding slik at den kan behandles trygt uten å forstyrres av nye mottak
    have_rx = false; // setter have_rx tilbake til false for å indikere at meldingen er mottatt og at det er klart til å motta en ny melding
    interrupts(); // Skrur tilbake alle maskinvare-avbrudd

    rx_count++; // øker rx_count for hver melding som blir motatt

    // Echo tilbake hva vi har mottatt
    Can0.write(m); // sender samme melding tilbake på CAN-bussen
    echo_count++; // øker echo_count for hver melding som blir echoed

    String data = bytesToHexString(m); // oversetter den kopierte meldingen

    // Printer i Serial Monitor mottatte meldignen
    Serial.print("[RX#"); Serial.print(rx_count); Serial.print("] "); // RX melding
    Serial.print("ID 0x"); Serial.print(m.id, HEX); // ID nummeret på meldignen
    Serial.print(m.flags.extended ? " (EXT)" : " (STD)"); // Viser om meldingen er standard (STD) eller utvidet (EXT)
    Serial.print(" DLC "); Serial.print(m.len); // Printer lengden på dataen
    Serial.print(" DATA "); Serial.println(data); // Printer informasjonen meldingen inneholder

    // Printer i Serial Monitor Echo meldigne
    Serial.print("[ECHO#"); Serial.print(echo_count); Serial.print("] "); // Echo melding
    Serial.print("ID 0x"); Serial.print(m.id, HEX); // ID nummeret på meldingen
    Serial.print(" DLC "); Serial.print(m.len); // Printer lengden på dataen
    Serial.print(" DATA "); Serial.println(data); 

    // Viser mottatte melding på OLED-skjermen
    if (display.width() > 0) // Kjører bare hvis OLED-skjermen fungerer
    {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      oledPrintLine(0, 0,  "Teensy 3.6 CAN demo"); // Printer en tittel øverst til venstre hjørne på skjermen
      oledPrintLine(0, 12, "RX ID: 0x" + String(m.id, HEX)); // Viser ID-en til mottatt melding
      oledPrintLine(0, 24, "DLC: " + String(m.len)); // Viser lengden på data
      oledPrintLine(0, 36, "DATA:"); // Skriver overskrift for datafeltet
      oledPrintLine(0, 48, data);  // Skriver selve dataen (i heksadesimal form)
    
      display.display();
    }
  }

// Oppsummering hvert 10. sekund:
// Viser antall sendte (TX), mottatte (RX) og echo-meldinger i Serial Monitor

  static elapsedMillis summaryTick; // Timer som teller millisekunder og beholder verdien mellom hver loop-iterasjon
  
  if (summaryTick >= 10000) {
    summaryTick = 0;
    Serial.print("[SUM] TX="); Serial.print(tx_count);
    Serial.print(" RX="); Serial.print(rx_count);
    Serial.print(" ECHO="); Serial.println(echo_count);
  }
}
