#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Stepper.h>

// ============================================================
// PARCHEGGIO / SBARRA - ARDUINO MEGA
// ============================================================
// ESP32 TX2 GPIO17 -> Mega RX1 pin 19
// Mega TX1 pin 18 -> partitore 1k/2k -> ESP32 RX2 GPIO16
HardwareSerial& espLink = Serial1;

const unsigned long LINK_BAUD = 9600UL;
const unsigned long SEND_STATE_MS = 1000UL;
const unsigned long DEBUG_STATUS_MS = 10000UL;
const bool DEBUG_VERBOSE = false;

// ============================================================
// HARDWARE
// ============================================================
#define SS_PIN 53
#define RST_PIN 30
MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal lcd(31, 33, 35, 37, 39, 41);

const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 11, 9, 10, 8);
const int passiSbarra = 512;

const int pinTrig1 = 2;
const int pinEcho1 = 3;
const int pinLed1 = 5;

const int pinTrig2 = 13;
const int pinEcho2 = 12;
const int pinLed2 = 4;

// ============================================================
// STATO APP / SERVER
// ============================================================
const int MAX_POSTI = 20;
const int sogliaDistanza = 15;

int parkingCapacity = MAX_POSTI;
int occupiedSpots = 0;
bool parkingBarrierOpen = false;
bool vehicleDetected = false;
bool rfidReaderEnabled = true;
bool lcdEnabled = true;

unsigned long lastStateSend = 0;
unsigned long lastDebugStatus = 0;
unsigned long stateSentCount = 0;
unsigned long commandReceivedCount = 0;
unsigned long ignoredCommandCount = 0;
bool lastLoggedBarrierOpen = false;
bool lastLoggedVehicleDetected = false;
int lastLoggedOccupiedSpots = -1;

int postiDisponibili();

void logLine(const __FlashStringHelper* area, const __FlashStringHelper* level, const String& message) {
  Serial.print('[');
  Serial.print(millis());
  Serial.print(F(" ms] PARK "));
  Serial.print(area);
  Serial.print(' ');
  Serial.print(level);
  Serial.print(F(" | "));
  Serial.println(message);
}

String boolText(bool value) {
  return value ? "1" : "0";
}

String stateSummary() {
  String msg = "spots=" + String(occupiedSpots) + "/" + String(parkingCapacity);
  msg += " free=" + String(postiDisponibili());
  msg += " barrier=" + boolText(parkingBarrierOpen);
  msg += " vehicle=" + boolText(vehicleDetected);
  msg += " rfid=" + boolText(rfidReaderEnabled);
  msg += " lcd=" + boolText(lcdEnabled);
  msg += " tx=" + String(stateSentCount);
  msg += " cmd=" + String(commandReceivedCount);
  msg += " ignored=" + String(ignoredCommandCount);
  return msg;
}

void logStateChanges() {
  if (parkingBarrierOpen != lastLoggedBarrierOpen) {
    logLine(F("BARRIER"), F("STATE"), parkingBarrierOpen ? F("aperta") : F("chiusa"));
    lastLoggedBarrierOpen = parkingBarrierOpen;
  }
  if (vehicleDetected != lastLoggedVehicleDetected) {
    logLine(F("SENSOR"), F("STATE"), vehicleDetected ? F("veicolo rilevato") : F("nessun veicolo"));
    lastLoggedVehicleDetected = vehicleDetected;
  }
  if (occupiedSpots != lastLoggedOccupiedSpots) {
    logLine(F("SPOTS"), F("STATE"), String(occupiedSpots) + "/" + String(parkingCapacity));
    lastLoggedOccupiedSpots = occupiedSpots;
  }
}

int postiDisponibili() {
  return max(0, parkingCapacity - occupiedSpots);
}

String valueOf(const String& line, const char* key) {
  const String token = String(key) + "=";
  const int start = line.indexOf(token);
  if (start < 0) {
    return "";
  }

  const int valueStart = start + token.length();
  int valueEnd = line.indexOf(';', valueStart);
  if (valueEnd < 0) {
    valueEnd = line.length();
  }

  return line.substring(valueStart, valueEnd);
}

bool readBoolField(const String& line, const char* key, bool fallback) {
  const String raw = valueOf(line, key);
  if (raw.length() == 0) {
    return fallback;
  }
  return raw == "1" || raw == "true" || raw == "TRUE";
}

int readIntField(const String& line, const char* key, int fallback) {
  const String raw = valueOf(line, key);
  if (raw.length() == 0) {
    return fallback;
  }
  return raw.toInt();
}

void applyCommandLine(const String& line) {
  if (!line.startsWith("CMD;")) {
    ignoredCommandCount++;
    if (DEBUG_VERBOSE) logLine(F("CMD"), F("SKIP"), line);
    return;
  }

  commandReceivedCount++;
  logLine(F("CMD"), F("RX"), line);

  rfidReaderEnabled = readBoolField(line, "rfidReaderEnabled", rfidReaderEnabled);
  lcdEnabled = readBoolField(line, "lcdEnabled", lcdEnabled);
  parkingCapacity = readIntField(line, "parkingCapacity", parkingCapacity);
  occupiedSpots = constrain(readIntField(line, "occupiedSpots", occupiedSpots), 0, parkingCapacity);

  const bool requestedBarrier = readBoolField(line, "parkingBarrierOpen", parkingBarrierOpen);
  if (requestedBarrier != parkingBarrierOpen) {
    requestedBarrier ? apriSbarra() : chiudiSbarra();
  }
}

void readSerialCommands() {
  static String buffer;

  while (espLink.available() > 0) {
    const char c = (char)espLink.read();
    if (c == '\n') {
      buffer.trim();
      applyCommandLine(buffer);
      buffer = "";
    } else if (c != '\r') {
      buffer += c;
      if (buffer.length() > 220) {
        buffer = "";
      }
    }
  }
}

void sendStateToEsp32() {
  espLink.print(F("STATE;"));
  espLink.print(F("rfidReaderEnabled="));
  espLink.print(rfidReaderEnabled ? 1 : 0);
  espLink.print(F(";parkingBarrierOpen="));
  espLink.print(parkingBarrierOpen ? 1 : 0);
  espLink.print(F(";vehicleDetected="));
  espLink.print(vehicleDetected ? 1 : 0);
  espLink.print(F(";parkingCapacity="));
  espLink.print(parkingCapacity);
  espLink.print(F(";occupiedSpots="));
  espLink.print(occupiedSpots);
  espLink.print(F(";lcdEnabled="));
  espLink.print(lcdEnabled ? 1 : 0);
  espLink.println(';');
  stateSentCount++;
  if (DEBUG_VERBOSE) logLine(F("SERIAL1"), F("TX"), stateSummary());
}

void serviceBridge() {
  readSerialCommands();

  const unsigned long now = millis();
  if (now - lastStateSend >= SEND_STATE_MS) {
    sendStateToEsp32();
    lastStateSend = now;
  }
}

void setup() {
  Serial.begin(115200);
  espLink.begin(LINK_BAUD);
  delay(200);
  logLine(F("BOOT"), F("INFO"), F("parcheggio_sbarra pronto"));
  logLine(F("BOOT"), F("PINS"), F("Mega Serial1: TX1 pin 18 -> ESP32 RX16, RX1 pin 19 <- ESP32 TX17"));
  logLine(F("BOOT"), F("BAUD"), String(LINK_BAUD));

  SPI.begin();
  mfrc522.PCD_Init();

  lcd.begin(16, 2);
  myStepper.setSpeed(15);

  pinMode(pinTrig1, OUTPUT);
  pinMode(pinEcho1, INPUT);
  pinMode(pinLed1, OUTPUT);
  pinMode(pinTrig2, OUTPUT);
  pinMode(pinEcho2, INPUT);
  pinMode(pinLed2, OUTPUT);

  stampaDefault();
  lastLoggedBarrierOpen = parkingBarrierOpen;
  lastLoggedVehicleDetected = vehicleDetected;
  lastLoggedOccupiedSpots = occupiedSpots;
  logLine(F("STATUS"), F("INFO"), stateSummary());
}

void loop() {
  serviceBridge();
  aggiornaLed();

  const long d1 = calcolaDistanza(pinTrig1, pinEcho1);
  const long d2 = calcolaDistanza(pinTrig2, pinEcho2);
  vehicleDetected = d1 < sogliaDistanza || d2 < sogliaDistanza;
  logStateChanges();

  const unsigned long now = millis();
  if (now - lastDebugStatus >= DEBUG_STATUS_MS) {
    String msg = stateSummary();
    msg += " d1=" + String(d1);
    msg += " d2=" + String(d2);
    logLine(F("STATUS"), F("INFO"), msg);
    lastDebugStatus = now;
  }

  if (d1 < sogliaDistanza && postiDisponibili() > 0) {
    mostraMessaggio("Auto ingresso", "Passa la card");

    while (calcolaDistanza(pinTrig1, pinEcho1) < sogliaDistanza) {
      serviceBridge();
      aggiornaLed();

      if (rfidReaderEnabled && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        mostraMessaggio("Card valida", "Apertura...");
        logLine(F("RFID"), F("ALLOW"), F("card valida ingresso"));
        apriSbarra();
        mostraMessaggio("Sbarra aperta", "Entra");

        while (calcolaDistanza(pinTrig2, pinEcho2) > sogliaDistanza) {
          serviceBridge();
          aggiornaLed();
        }

        while (calcolaDistanza(pinTrig2, pinEcho2) < sogliaDistanza) {
          serviceBridge();
          aggiornaLed();
        }

        chiudiSbarra();
        occupiedSpots = min(parkingCapacity, occupiedSpots + 1);
        mostraMessaggio("Ingresso OK", "Posti: " + String(postiDisponibili()));

        delay(1200);
        resetRFID();
        stampaDefault();
        break;
      }
    }

    stampaDefault();
  } else if (d2 < sogliaDistanza) {
    mostraMessaggio("Auto uscita", "Apertura...");
    apriSbarra();
    mostraMessaggio("Arrivederci", "Passare ora");

    while (calcolaDistanza(pinTrig1, pinEcho1) > sogliaDistanza) {
      serviceBridge();
      aggiornaLed();
    }

    while (calcolaDistanza(pinTrig1, pinEcho1) < sogliaDistanza) {
      serviceBridge();
      aggiornaLed();
    }

    chiudiSbarra();
    occupiedSpots = max(0, occupiedSpots - 1);
    mostraMessaggio("Uscita OK", "Posti: " + String(postiDisponibili()));

    delay(1200);
    stampaDefault();
  }
}

void aggiornaLed() {
  const long l1 = calcolaDistanza(pinTrig1, pinEcho1);
  const long l2 = calcolaDistanza(pinTrig2, pinEcho2);
  digitalWrite(pinLed1, l1 < sogliaDistanza ? HIGH : LOW);
  digitalWrite(pinLed2, l2 < sogliaDistanza ? HIGH : LOW);
}

long calcolaDistanza(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  const long durata = pulseIn(echo, HIGH, 20000);
  if (durata == 0) {
    return 100;
  }
  return durata / 58;
}

void apriSbarra() {
  if (parkingBarrierOpen) {
    return;
  }
  myStepper.step(passiSbarra);
  parkingBarrierOpen = true;
  logLine(F("BARRIER"), F("CMD"), F("apri"));
}

void chiudiSbarra() {
  if (!parkingBarrierOpen) {
    return;
  }
  myStepper.step(-passiSbarra);
  parkingBarrierOpen = false;
  logLine(F("BARRIER"), F("CMD"), F("chiudi"));
}

void mostraMessaggio(const String& r1, const String& r2) {
  if (!lcdEnabled) {
    lcd.noDisplay();
    return;
  }

  lcd.display();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(r1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(r2.substring(0, 16));
}

void stampaDefault() {
  mostraMessaggio("Parcheggio", "Posti: " + String(postiDisponibili()));
}

void resetRFID() {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
