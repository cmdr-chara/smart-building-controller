#include <Stepper.h>

// ============================================================
// ESTERNI / TENDA
// ============================================================
// Seriale hardware Arduino Uno:
// ESP32 TX2 GPIO17 -> Arduino RX pin 0
// Arduino TX pin 1 -> partitore 1k/2k -> ESP32 RX2 GPIO16
#define espLink Serial

const unsigned long LINK_BAUD = 9600UL;
const unsigned long SEND_STATE_MS = 1000UL;
const unsigned long COMMAND_TIMEOUT_MS = 10000UL;
const unsigned long DEBUG_STATUS_MS = 10000UL;
// Su Arduino Uno Serial e' anche il link con ESP32. Lascia false quando ESP32 e' collegato.
const bool DEBUG_TO_SERIAL = false;

const int passiPerGiro = 2048;
Stepper mioStepper(passiPerGiro, 8, 10, 9, 11);

const int PIN_LDR = A0;
const int PIN_LED_DA = 2;
const int PIN_LED_A = 5;

const int sogliaBuio = 600;
const int sogliaLuce = 400;
const int posizioneChiusa = passiPerGiro * 2;

int posizioneAttuale = 0;
int posizioneTarget = 0;
bool twilightDetected = false;
bool exteriorLightsOn = false;
bool awningOpen = true;
unsigned long lastStateSend = 0;
unsigned long lastCommandReceived = 0;
unsigned long lastDebugStatus = 0;
unsigned long stateSentCount = 0;
unsigned long commandReceivedCount = 0;
unsigned long ignoredCommandCount = 0;
int lastLoggedLuminosita = -1;

void debugLine(const __FlashStringHelper* tag, const String& message) {
  if (!DEBUG_TO_SERIAL) return;
  Serial.print('[');
  Serial.print(millis());
  Serial.print(F(" ms] ESTERNI "));
  Serial.print(tag);
  Serial.print(F(" | "));
  Serial.println(message);
}

void debugStatus(int luminosita) {
  if (!DEBUG_TO_SERIAL) return;
  String msg = "ldr=" + String(luminosita);
  msg += " twilight=" + String(twilightDetected ? 1 : 0);
  msg += " lights=" + String(exteriorLightsOn ? 1 : 0);
  msg += " awning=" + String(awningOpen ? 1 : 0);
  msg += " pos=" + String(posizioneAttuale) + "/" + String(posizioneTarget);
  msg += " tx=" + String(stateSentCount);
  msg += " cmd=" + String(commandReceivedCount);
  msg += " ignored=" + String(ignoredCommandCount);
  debugLine(F("STATUS"), msg);
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

void setExteriorLights(bool enabled) {
  exteriorLightsOn = enabled;
  for (int i = PIN_LED_DA; i <= PIN_LED_A; i++) {
    digitalWrite(i, enabled ? HIGH : LOW);
  }
}

void setAwningOpen(bool open) {
  awningOpen = open;
  posizioneTarget = open ? 0 : posizioneChiusa;
}

void applyCommandLine(const String& line) {
  if (!line.startsWith("CMD;")) {
    ignoredCommandCount++;
    debugLine(F("CMD SKIP"), line);
    return;
  }

  const bool hasExteriorCommand = valueOf(line, "exteriorLightsOn").length() > 0;
  const bool hasAwningCommand = valueOf(line, "awningOpen").length() > 0;
  if (!hasExteriorCommand && !hasAwningCommand) {
    ignoredCommandCount++;
    debugLine(F("CMD EMPTY"), line);
    return;
  }

  if (hasExteriorCommand) {
    setExteriorLights(readBoolField(line, "exteriorLightsOn", exteriorLightsOn));
  }
  if (hasAwningCommand) {
    setAwningOpen(readBoolField(line, "awningOpen", awningOpen));
  }
  lastCommandReceived = millis();
  commandReceivedCount++;
  debugLine(F("CMD RX"), line);
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
      if (buffer.length() > 180) {
        buffer = "";
      }
    }
  }
}

void sendStateToEsp32() {
  espLink.print(F("STATE;"));
  espLink.print(F("twilightDetected="));
  espLink.print(twilightDetected ? 1 : 0);
  espLink.print(F(";exteriorLightsOn="));
  espLink.print(exteriorLightsOn ? 1 : 0);
  espLink.print(F(";awningOpen="));
  espLink.print(awningOpen ? 1 : 0);
  espLink.println(';');
  stateSentCount++;
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
  for (int i = PIN_LED_DA; i <= PIN_LED_A; i++) {
    pinMode(i, OUTPUT);
  }

  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);

  mioStepper.setSpeed(10);
  espLink.begin(LINK_BAUD);

  setExteriorLights(false);
  setAwningOpen(true);
  debugLine(F("BOOT"), F("Arduino Uno Serial pin 0 RX, pin 1 TX verso ESP32; scollega 0/1 durante upload"));
}

void loop() {
  serviceBridge();

  const int luminosita = analogRead(PIN_LDR);

  if (millis() - lastCommandReceived > COMMAND_TIMEOUT_MS) {
    if (luminosita > sogliaBuio) {
      twilightDetected = true;
      setExteriorLights(true);
      setAwningOpen(false);
    } else if (luminosita < sogliaLuce) {
      twilightDetected = false;
      setExteriorLights(false);
      setAwningOpen(true);
    }
  }

  if (DEBUG_TO_SERIAL && (millis() - lastDebugStatus >= DEBUG_STATUS_MS || abs(luminosita - lastLoggedLuminosita) > 80)) {
    debugStatus(luminosita);
    lastDebugStatus = millis();
    lastLoggedLuminosita = luminosita;
  }

  updateAwningMotor();
}

void updateAwningMotor() {
  if (posizioneAttuale < posizioneTarget) {
    mioStepper.step(-1);
    posizioneAttuale++;
  } else if (posizioneAttuale > posizioneTarget) {
    mioStepper.step(1);
    posizioneAttuale--;
  } else {
    rilasciaMotore();
  }
}

void rilasciaMotore() {
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
}
