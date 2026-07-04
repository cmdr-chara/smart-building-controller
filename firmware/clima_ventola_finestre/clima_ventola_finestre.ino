#define SSD1306_NO_SPLASH

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT11.h>
#include <Stepper.h>

// ============================================================
// SERIALE verso ESP32
// ============================================================
// Arduino Mega usa Serial1 hardware:
// ESP32 TX2 GPIO17 -> Mega RX1 pin 19
// Mega TX1 pin 18 -> partitore 1k/2k -> ESP32 RX2 GPIO16
#define espLink Serial1

// ============================================================
// OLED
// ============================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================
// HARDWARE
// ============================================================
const uint8_t PIN_DHT = 8;
const uint8_t PIN_FAN = 9;
const uint8_t PIN_BUZZER = 10;
DHT11 dht11(PIN_DHT);

const uint16_t NOTE_E2 = 82;
const uint16_t NOTE_AS2 = 117;
const uint16_t NOTE_B2 = 123;
const uint16_t NOTE_AS3 = 233;
const uint16_t NOTE_B3 = 247;
const uint16_t NOTE_C3 = 131;
const uint16_t NOTE_D3 = 147;
const uint16_t NOTE_E3 = 165;
const uint16_t NOTE_REST = 0;
const uint16_t NOTE_A2 = 110;
const uint16_t NOTE_C4 = 262;
const uint16_t NOTE_CS3 = 139;
const uint16_t NOTE_DS3 = 156;
const uint16_t NOTE_F3 = 175;
const uint16_t NOTE_FS3 = 185;
const uint16_t NOTE_G3 = 196;
const uint16_t NOTE_GS3 = 208;
const uint16_t NOTE_A3 = 220;
const uint16_t NOTE_D4 = 294;
const uint16_t NOTE_CS4 = 277;
const uint16_t NOTE_DS4 = 311;
const uint16_t NOTE_E4 = 330;
const uint16_t NOTE_F4 = 349;
const uint16_t NOTE_FS4 = 370;
const uint16_t NOTE_G4 = 392;
const uint16_t NOTE_GS4 = 415;
const uint16_t NOTE_A4 = 440;
const uint16_t NOTE_AS4 = 466;
const uint16_t NOTE_B4 = 494;
const uint16_t NOTE_C5 = 523;
const uint16_t NOTE_CS5 = 554;
const uint16_t NOTE_D5 = 587;
const uint16_t NOTE_DS5 = 622;
const uint16_t NOTE_E5 = 659;
const uint16_t NOTE_F5 = 698;
const uint16_t NOTE_FS5 = 740;
const uint16_t NOTE_G5 = 784;
const uint16_t NOTE_GS5 = 831;
const uint16_t NOTE_A5 = 880;
const uint16_t NOTE_AS5 = 932;
const uint16_t NOTE_B5 = 988;
const uint16_t NOTE_C6 = 1047;
const uint16_t NOTE_D6 = 1175;
const uint16_t NOTE_E6 = 1319;
const uint16_t NOTE_F6 = 1397;

// Motore finestra ULN2003
const int PASSI_PER_GIRO = 2048;
Stepper windowMotor(PASSI_PER_GIRO, A0, A1, A2, A3);
const int PASSI_SPORTELLO = 512;

// ============================================================
// CONFIG VENTOLA - PARTE IMPORTANTE
// ============================================================
// false = ventola pilotata normalmente con MOSFET/transistor:
//         power 0 = OFF, power 255 = MAX
//
// true  = modulo active-low:
//         power 0 = OFF diventa PWM 255
//         power 255 = MAX diventa PWM 0
//
// Se la ventola NON parte, prova a cambiare false -> true.
const bool FAN_ACTIVE_LOW = false;

// Potenza normale dopo lo start boost.
const uint8_t FAN_RUN_POWER = 160;

// Boost iniziale per far partire ventole pigre.
const uint8_t FAN_START_POWER = 160;
const unsigned long FAN_START_BOOST_MS = 900UL;

// Diagnostica all'avvio: lasciata disattivata per non far partire ventola/buzzer.
const bool FAN_SELF_TEST_ON_BOOT = false;

// ============================================================
// PARAMETRI CLIMA
// ============================================================
const int SOGLIA_VENTOLA_ON = 24;
const int SOGLIA_VENTOLA_OFF = 23;

const int TEMP_MIN = 15; 
const int TEMP_MAX = 40;

// ============================================================
// TEMPI
// ============================================================
const unsigned long SENSOR_INTERVAL_MS = 2000UL;
const unsigned long STATE_INTERVAL_MS = 2000UL;
const unsigned long UI_ANIM_MS = 180UL;
const unsigned long TIMEOUT_COMANDI = 10000UL;
const unsigned long RX_FLASH_MS = 420UL;
const unsigned long DEBUG_STATUS_MS = 10000UL;
const unsigned long BUZZER_COOLDOWN_MS = 650UL;

const bool DEBUG_VERBOSE = false;
const bool BUZZER_ENABLED = true;

unsigned long lastSensorRead = 0;
unsigned long lastStateSend = 0;
unsigned long lastUiAnim = 0;
unsigned long lastDebugStatus = 0;
unsigned long stateSentCount = 0;
unsigned long commandReceivedCount = 0;
unsigned long ignoredCommandCount = 0;
unsigned long lastAnyCommandReceived = 0;
unsigned long lastClimateCommandReceived = 0;
unsigned long fanBoostUntil = 0;
unsigned long lastBuzzerAt = 0;
int currentBuzzerSpeed = 100;
bool buzzerPlaybackActive = false;
bool buzzerStopRequested = false;

void handleIncomingCommands();

// ============================================================
// STATO SENSORI / ATTUATORI
// ============================================================
int temperature = 0;
int humidity = 0;
bool sensorOk = false;

bool fanOn = false;
bool windowsOpen = false;
bool windowMotorOpen = false;

uint8_t uiFrame = 0;
uint8_t fanFrame = 0;

uint8_t lastFanPower = 0;
uint8_t lastFanPwm = 0;
bool lastLoggedSensorOk = true;
bool lastLoggedFanOn = false;
bool lastLoggedWindowsOpen = false;

// ============================================================
// STATO APP / DOMOTICA
// ============================================================
bool internalDoorUnlocked = false;
bool intrusionAlarmArmed = true;
bool rfidReaderEnabled = true;
bool parkingBarrierOpen = false;
bool vehicleDetected = false;
int parkingCapacity = 32;
int occupiedSpots = 0;
bool twilightDetected = false;
bool exteriorLightsOn = false;
bool awningOpen = true;
bool motionDetected = false;
bool indoorLightsOn = false;
bool lcdEnabled = true;

// ============================================================
// STORICO GRAFICO
// ============================================================
const uint8_t HISTORY_SIZE = 48;
int8_t tempHistory[HISTORY_SIZE];
int8_t humHistory[HISTORY_SIZE];
uint8_t historyIndex = 0;
bool historyReady = false;

const bool OLED_EXTREME_FX = false;

// ============================================================
// UTILITY
// ============================================================
int mapClamped(int value, int inMin, int inMax, int outMin, int outMax) {
  value = constrain(value, inMin, inMax);
  return map(value, inMin, inMax, outMin, outMax);
}

bool remoteClimateOverrideActive() {
  return lastClimateCommandReceived != 0 &&
         millis() - lastClimateCommandReceived <= TIMEOUT_COMANDI;
}

void resetText() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
}

void initHistory() {
  for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
    tempHistory[i] = temperature;
    humHistory[i] = humidity;
  }

  historyIndex = 0;
  historyReady = true;
}

void pushHistory() {
  if (!sensorOk) return;
  if (!historyReady) initHistory();

  tempHistory[historyIndex] = constrain(temperature, -20, 80);
  humHistory[historyIndex] = constrain(humidity, 0, 100);

  historyIndex++;
  if (historyIndex >= HISTORY_SIZE) historyIndex = 0;
}

void logTag(const __FlashStringHelper* tag) {
  Serial.print(F("[CLIMA "));
  Serial.print(tag);
  Serial.print(F("] "));
}

void logStatusSummary() {
  logTag(F("STATUS"));
  Serial.print(F("dht="));
  Serial.print(sensorOk ? F("ok") : F("err"));
  Serial.print(F(" temp="));
  Serial.print(temperature);
  Serial.print(F("C hum="));
  Serial.print(humidity);
  Serial.print(F("% fan="));
  Serial.print(fanOn ? F("on") : F("off"));
  Serial.print(F(" power="));
  Serial.print(lastFanPower);
  Serial.print(F(" pwm="));
  Serial.print(lastFanPwm);
  Serial.print(F(" windows="));
  Serial.print(windowsOpen ? F("open") : F("closed"));
  Serial.print(F(" mode="));
  Serial.print(remoteClimateOverrideActive() ? F("esp32") : F("auto"));
  Serial.print(F(" tx="));
  Serial.print(stateSentCount);
  Serial.print(F(" cmd="));
  Serial.print(commandReceivedCount);
  Serial.print(F(" ignored="));
  Serial.println(ignoredCommandCount);
}

void logStateChanges() {
  if (sensorOk != lastLoggedSensorOk) {
    logTag(F("DHT"));
    Serial.println(sensorOk ? F("lettura ripristinata") : F("lettura fallita"));
    lastLoggedSensorOk = sensorOk;
  }

  if (fanOn != lastLoggedFanOn) {
    logTag(F("FAN"));
    Serial.println(fanOn ? F("ON") : F("OFF"));
    lastLoggedFanOn = fanOn;
  }

  if (windowsOpen != lastLoggedWindowsOpen) {
    logTag(F("WINDOW"));
    Serial.println(windowsOpen ? F("APERTE") : F("CHIUSE"));
    lastLoggedWindowsOpen = windowsOpen;
  }
}

void beepEvent(uint16_t frequency, uint16_t durationMs) {
  if (!BUZZER_ENABLED) return;
  if (millis() - lastBuzzerAt < BUZZER_COOLDOWN_MS) return;

  uint16_t low = max(220, frequency - 120);
  uint16_t high = frequency + 95;
  uint16_t noteMs = max((uint16_t)24, (uint16_t)(durationMs / 2));

  tone(PIN_BUZZER, low, noteMs);
  delay(noteMs + 14);
  tone(PIN_BUZZER, frequency, noteMs);
  delay(noteMs + 14);
  tone(PIN_BUZZER, high, noteMs);
  delay(noteMs + 10);
  noTone(PIN_BUZZER);

  lastBuzzerAt = millis();
}

uint16_t scaledBuzzerDuration(uint16_t durationMs) {
  long scaled = (long)durationMs * 100L / constrain(currentBuzzerSpeed, 50, 200);
  return (uint16_t)constrain(scaled, 8L, 2000L);
}

void beginBuzzerPlayback() {
  buzzerPlaybackActive = true;
  buzzerStopRequested = false;
}

void endBuzzerPlayback() {
  noTone(PIN_BUZZER);
  buzzerPlaybackActive = false;
  buzzerStopRequested = false;
  lastBuzzerAt = millis();
}

bool waitBuzzerDuration(uint16_t durationMs) {
  const unsigned long start = millis();

  while ((unsigned long)(millis() - start) < durationMs) {
    if (buzzerStopRequested) {
      noTone(PIN_BUZZER);
      return false;
    }

    handleIncomingCommands();

    if (buzzerStopRequested) {
      noTone(PIN_BUZZER);
      return false;
    }

    const unsigned long elapsed = millis() - start;
    if (elapsed >= durationMs) break;
    delay(min((unsigned long)10, (unsigned long)durationMs - elapsed));
  }

  return !buzzerStopRequested;
}

void playMelody(const uint16_t notes[], const uint16_t durations[], uint8_t count, uint16_t gapMs) {
  if (!BUZZER_ENABLED) return;

  beginBuzzerPlayback();

  for (uint8_t i = 0; i < count; i++) {
    if (buzzerStopRequested) break;

    const uint16_t note = notes[i];
    const uint16_t durationMs = scaledBuzzerDuration(durations[i]);
    const uint16_t scaledGapMs = scaledBuzzerDuration(gapMs);
    if (note != NOTE_REST) {
      tone(PIN_BUZZER, note, durationMs > scaledGapMs ? durationMs - scaledGapMs : durationMs);
    }
    if (!waitBuzzerDuration(durationMs)) break;
    noTone(PIN_BUZZER);
    if (!waitBuzzerDuration(scaledGapMs)) break;
  }

  endBuzzerPlayback();
}

void playProgmemMelody(
  const uint16_t notes[],
  const uint16_t durations[],
  uint16_t count,
  uint16_t gapMs
) {
  if (!BUZZER_ENABLED) return;

  beginBuzzerPlayback();

  for (uint16_t i = 0; i < count; i++) {
    if (buzzerStopRequested) break;

    const uint16_t note = pgm_read_word_near(notes + i);
    const uint16_t durationMs = scaledBuzzerDuration(pgm_read_word_near(durations + i));
    const uint16_t scaledGapMs = scaledBuzzerDuration(gapMs);
    if (note != NOTE_REST) {
      tone(PIN_BUZZER, note, durationMs > scaledGapMs ? durationMs - scaledGapMs : durationMs);
    }
    if (!waitBuzzerDuration(durationMs)) break;
    noTone(PIN_BUZZER);
    if (!waitBuzzerDuration(scaledGapMs)) break;
  }

  endBuzzerPlayback();
}

void playDoomBuzzer() {
  // At Doom's Gate/E1M1 Arduino transcription, matching the common
  // Musescore-derived buzzer array used in Wokwi/Arduino examples.
  static const int16_t melody[] PROGMEM = {
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_FS3, -16, NOTE_D3, -16, NOTE_B2, -16, NOTE_A3, -16, NOTE_FS3, -16, NOTE_B2, -16, NOTE_D3, -16, NOTE_FS3, -16, NOTE_A3, -16, NOTE_FS3, -16, NOTE_D3, -16, NOTE_B2, -16,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_B3, -16, NOTE_G3, -16, NOTE_E3, -16, NOTE_G3, -16, NOTE_B3, -16, NOTE_E4, -16, NOTE_G3, -16, NOTE_B3, -16, NOTE_E4, -16, NOTE_B3, -16, NOTE_G4, -16, NOTE_B4, -16,

    NOTE_A2, 8, NOTE_A2, 8, NOTE_A3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_G3, 8, NOTE_A2, 8, NOTE_A2, 8,
    NOTE_F3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_DS3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_E3, 8, NOTE_F3, 8,
    NOTE_A2, 8, NOTE_A2, 8, NOTE_A3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_G3, 8, NOTE_A2, 8, NOTE_A2, 8,
    NOTE_F3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_DS3, -2,

    NOTE_A2, 8, NOTE_A2, 8, NOTE_A3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_G3, 8, NOTE_A2, 8, NOTE_A2, 8,
    NOTE_F3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_DS3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_E3, 8, NOTE_F3, 8,
    NOTE_A2, 8, NOTE_A2, 8, NOTE_A3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_G3, 8, NOTE_A2, 8, NOTE_A2, 8,
    NOTE_A3, -16, NOTE_F3, -16, NOTE_D3, -16, NOTE_A3, -16, NOTE_F3, -16, NOTE_D3, -16, NOTE_C4, -16, NOTE_A3, -16, NOTE_F3, -16, NOTE_A3, -16, NOTE_F3, -16, NOTE_D3, -16,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_CS3, 8, NOTE_CS3, 8, NOTE_CS4, 8, NOTE_CS3, 8, NOTE_CS3, 8, NOTE_B3, 8, NOTE_CS3, 8, NOTE_CS3, 8,
    NOTE_A3, 8, NOTE_CS3, 8, NOTE_CS3, 8, NOTE_G3, 8, NOTE_CS3, 8, NOTE_CS3, 8, NOTE_GS3, 8, NOTE_A3, 8,
    NOTE_B2, 8, NOTE_B2, 8, NOTE_B3, 8, NOTE_B2, 8, NOTE_B2, 8, NOTE_A3, 8, NOTE_B2, 8, NOTE_B2, 8,
    NOTE_G3, 8, NOTE_B2, 8, NOTE_B2, 8, NOTE_F3, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_B3, -16, NOTE_G3, -16, NOTE_E3, -16, NOTE_G3, -16, NOTE_B3, -16, NOTE_E4, -16, NOTE_G3, -16, NOTE_B3, -16, NOTE_E4, -16, NOTE_B3, -16, NOTE_G4, -16, NOTE_B4, -16,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_FS3, -16, NOTE_DS3, -16, NOTE_B2, -16, NOTE_FS3, -16, NOTE_DS3, -16, NOTE_B2, -16, NOTE_G3, -16, NOTE_D3, -16, NOTE_B2, -16, NOTE_DS4, -16, NOTE_DS3, -16, NOTE_B2, -16,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_E4, -16, NOTE_B3, -16, NOTE_G3, -16, NOTE_G4, -16, NOTE_E4, -16, NOTE_G3, -16, NOTE_B3, -16, NOTE_D4, -16, NOTE_E4, -16, NOTE_G4, -16, NOTE_E4, -16, NOTE_G3, -16,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_A2, 8, NOTE_A2, 8, NOTE_A3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_G3, 8, NOTE_A2, 8, NOTE_A2, 8,
    NOTE_F3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_DS3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_E3, 8, NOTE_F3, 8,
    NOTE_A2, 8, NOTE_A2, 8, NOTE_A3, 8, NOTE_A2, 8, NOTE_A2, 8, NOTE_G3, 8, NOTE_A2, 8, NOTE_A2, 8,
    NOTE_A3, -16, NOTE_F3, -16, NOTE_D3, -16, NOTE_A3, -16, NOTE_F3, -16, NOTE_D3, -16, NOTE_C4, -16, NOTE_A3, -16, NOTE_F3, -16, NOTE_A3, -16, NOTE_F3, -16, NOTE_D3, -16,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
    NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
    NOTE_B3, -16, NOTE_G3, -16, NOTE_E3, -16, NOTE_B2, -16, NOTE_E3, -16, NOTE_G3, -16, NOTE_C4, -16, NOTE_B3, -16, NOTE_G3, -16, NOTE_B3, -16, NOTE_G3, -16, NOTE_E3, -16,
  };
  const uint16_t pairCount = sizeof(melody) / sizeof(melody[0]) / 2;
  const uint16_t wholeNoteMs = scaledBuzzerDuration((60000UL * 4UL) / 225UL);

  if (!BUZZER_ENABLED) return;

  beginBuzzerPlayback();

  for (uint16_t i = 0; i < pairCount * 2; i += 2) {
    if (buzzerStopRequested) break;

    const uint16_t note = pgm_read_word_near(melody + i);
    int16_t divider = pgm_read_word_near(melody + i + 1);
    uint16_t durationMs = wholeNoteMs / abs(divider);
    if (divider < 0) {
      durationMs += durationMs / 2;
    }

    if (note != NOTE_REST) {
      tone(PIN_BUZZER, note, durationMs * 9 / 10);
    }
    if (!waitBuzzerDuration(durationMs)) break;
    noTone(PIN_BUZZER);
  }

  endBuzzerPlayback();
}

void playToreadorMarch() {
  // Flutetunes Toreador Song F-major excerpt, reduced to a single piezo line.
  static const uint16_t notes[] PROGMEM = {
    NOTE_C6, NOTE_D6, NOTE_C6, NOTE_A5, NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_G5,
    NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_AS5, NOTE_G5, NOTE_C6, NOTE_A5, NOTE_F5,
    NOTE_D5, NOTE_G5, NOTE_C5, NOTE_G5, NOTE_G5, NOTE_D6, NOTE_C6, NOTE_AS5,
    NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_E5, NOTE_A5,
    NOTE_A5, NOTE_GS5, NOTE_B5,
  };
  static const uint16_t steps[] PROGMEM = {
    556, 417, 139, 556, 524, 32, 417, 139,
    417, 139, 1111, 556, 417, 139, 1111, 556,
    417, 139, 1111, 1111, 278, 278, 278, 246,
    32, 278, 278, 278, 278, 1111, 556, 556,
    556, 278, 278,
  };
  static const uint8_t gates[] PROGMEM = {
    100, 100, 100, 75, 100, 95, 100, 100, 100, 100, 75, 100, 100, 100, 75, 100,
    100, 100, 75, 100, 100, 100, 100, 100, 95, 100, 100, 100, 100, 75, 100, 100,
    100, 100, 100,
  };
  const uint8_t count = sizeof(notes) / sizeof(notes[0]);

  if (!BUZZER_ENABLED) return;

  beginBuzzerPlayback();

  for (uint8_t i = 0; i < count; i++) {
    if (buzzerStopRequested) break;

    const uint16_t note = pgm_read_word_near(notes + i);
    const uint16_t stepMs = scaledBuzzerDuration(pgm_read_word_near(steps + i));
    const uint16_t toneMs = stepMs * pgm_read_byte_near(gates + i) / 100;
    tone(PIN_BUZZER, note, toneMs);
    if (!waitBuzzerDuration(stepMs)) break;
    noTone(PIN_BUZZER);
  }

  endBuzzerPlayback();
}

void playFnafMusicBox() {
  // Henry Clay Work's public-domain "My Grandfather's Clock".
  // Full verse plus chorus from John Chambers' G-major ABC transcription, shifted up.
  static const uint16_t notes[] PROGMEM = {
    NOTE_D5,
    NOTE_G5, NOTE_FS5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_A5,
    NOTE_B5, NOTE_C6, NOTE_B5, NOTE_E5, NOTE_A5, NOTE_A5,
    NOTE_G5, NOTE_G5, NOTE_G5, NOTE_FS5, NOTE_E5, NOTE_FS5,
    NOTE_G5, NOTE_REST,

    NOTE_D5,
    NOTE_G5, NOTE_FS5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_A5,
    NOTE_B5, NOTE_C6, NOTE_B5, NOTE_E5, NOTE_A5, NOTE_A5,
    NOTE_G5, NOTE_G5, NOTE_G5, NOTE_FS5, NOTE_E5, NOTE_FS5,
    NOTE_G5, NOTE_REST,

    NOTE_G5, NOTE_B5,
    NOTE_D6, NOTE_B5, NOTE_A5, NOTE_G5, NOTE_FS5, NOTE_G5,
    NOTE_A5, NOTE_G5, NOTE_FS5, NOTE_E5, NOTE_D5, NOTE_G5, NOTE_B5,
    NOTE_D6, NOTE_B5, NOTE_A5, NOTE_G5, NOTE_FS5, NOTE_G5,
    NOTE_A5, NOTE_REST, NOTE_D5, NOTE_D5,
    NOTE_G5, NOTE_REST, NOTE_A5, NOTE_REST,
    NOTE_B5, NOTE_B5, NOTE_B5, NOTE_C6, NOTE_B5, NOTE_E5, NOTE_A5, NOTE_A5,
    NOTE_G5, NOTE_FS5, NOTE_G5, NOTE_REST,

    NOTE_D5, NOTE_D5,
    NOTE_G5, NOTE_D5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_D5,
    NOTE_B4, NOTE_REST, NOTE_D5, NOTE_REST, NOTE_B4, NOTE_REST, NOTE_D5, NOTE_D5,
    NOTE_G5, NOTE_D5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_D5,
    NOTE_B4, NOTE_REST, NOTE_D5, NOTE_REST, NOTE_B4, NOTE_REST, NOTE_D5, NOTE_D5,
    NOTE_G5, NOTE_REST, NOTE_A5, NOTE_REST,
    NOTE_B5, NOTE_B5, NOTE_B5, NOTE_C6, NOTE_B5, NOTE_E5, NOTE_A5, NOTE_A5,
    NOTE_G5, NOTE_FS5, NOTE_G5, NOTE_REST,
  };
  static const uint16_t durations[] PROGMEM = {
    360,
    360, 180, 180, 360, 180, 180,
    360, 180, 180, 360, 180, 180,
    360, 180, 180, 360, 180, 180,
    720, 360,

    360,
    360, 180, 180, 360, 180, 180,
    360, 180, 180, 360, 180, 180,
    360, 180, 180, 360, 180, 180,
    720, 360,

    180, 180,
    360, 180, 180, 360, 180, 180,
    180, 180, 180, 180, 360, 180, 180,
    360, 180, 180, 360, 180, 180,
    720, 360, 180, 180,
    360, 360, 360, 360,
    120, 120, 120, 180, 180, 360, 180, 180,
    720, 720, 720, 360,

    180, 180,
    360, 180, 180, 270, 90, 360,
    180, 180, 180, 180, 180, 180, 180, 180,
    360, 180, 180, 270, 90, 360,
    180, 180, 180, 180, 180, 180, 180, 180,
    360, 360, 360, 360,
    120, 120, 120, 180, 180, 360, 180, 180,
    720, 720, 720, 360,
  };

  playProgmemMelody(notes, durations, sizeof(notes) / sizeof(notes[0]), 20);
}

void playMegaBossPattern() {
  if (!BUZZER_ENABLED) return;

  // Compact piezo transcription based on the common Arduino/Reddit/GitHub
  // Megalovania buzzer map, reduced to the recognizable opening riff.
  static const uint16_t notes[] PROGMEM = {
    NOTE_D4, NOTE_D4, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
    NOTE_C4, NOTE_C4, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
    NOTE_B3, NOTE_B3, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
    NOTE_AS3, NOTE_AS3, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,

    NOTE_D4, NOTE_D4, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
    NOTE_C4, NOTE_C4, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
    NOTE_B3, NOTE_B3, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
    NOTE_AS3, NOTE_AS3, NOTE_D5, NOTE_A4, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS4, NOTE_G4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,

    NOTE_D5, NOTE_D5, NOTE_D6, NOTE_A5, NOTE_REST, NOTE_REST, NOTE_REST, NOTE_GS5, NOTE_G5, NOTE_F5, NOTE_D5, NOTE_F5, NOTE_G5,
  };
  static const uint8_t dividers[] PROGMEM = {
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,

    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,

    16, 16, 8, 8, 64, 64, 32, 8, 8, 8, 16, 16, 16,
  };
  static const uint8_t gates[] PROGMEM = {
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,

    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,

    50, 50, 50, 50, 0, 0, 0, 50, 50, 80, 50, 50, 50,
  };
  const uint16_t wholeNoteMs = scaledBuzzerDuration(2000);
  const uint8_t count = sizeof(notes) / sizeof(notes[0]);

  beginBuzzerPlayback();

  for (uint8_t i = 0; i < count; i++) {
    if (buzzerStopRequested) break;

    const uint16_t note = pgm_read_word_near(notes + i);
    const uint16_t noteMs = wholeNoteMs / pgm_read_byte_near(dividers + i);
    const uint16_t toneMs = noteMs * pgm_read_byte_near(gates + i) / 100;
    if (note != NOTE_REST) {
      tone(PIN_BUZZER, note, toneMs);
    }
    if (!waitBuzzerDuration(noteMs)) break;
    noTone(PIN_BUZZER);
  }

  endBuzzerPlayback();
}

void playItalianAmbulanceSiren() {
  if (!BUZZER_ENABLED) return;

  beginBuzzerPlayback();

  for (uint8_t cycle = 0; cycle < 7; cycle++) {
    if (buzzerStopRequested) break;
    tone(PIN_BUZZER, 435, scaledBuzzerDuration(520));
    if (!waitBuzzerDuration(scaledBuzzerDuration(560))) break;
    tone(PIN_BUZZER, 580, scaledBuzzerDuration(520));
    if (!waitBuzzerDuration(scaledBuzzerDuration(560))) break;
  }

  endBuzzerPlayback();
}

// ============================================================
// VENTOLA - FIX VERO
// ============================================================
uint8_t fanPowerToPwm(uint8_t power) {
  if (FAN_ACTIVE_LOW) {
    return 255 - power;
  }

  return power;
}

void writeFanPower(uint8_t power, const __FlashStringHelper* reason) {
  uint8_t pwm = fanPowerToPwm(power);
  bool changed = power != lastFanPower || pwm != lastFanPwm;

  lastFanPower = power;
  lastFanPwm = pwm;

  if (power == 0) {
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, FAN_ACTIVE_LOW ? HIGH : LOW);
} else {
  analogWrite(PIN_FAN, pwm);
}

  if (!DEBUG_VERBOSE && !changed) return;

  Serial.print(F("[FAN] "));
  Serial.print(reason);
  Serial.print(F(" | power="));
  Serial.print(power);
  Serial.print(F(" | pwm="));
  Serial.print(pwm);
  Serial.print(F(" | activeLow="));
  Serial.println(FAN_ACTIVE_LOW ? F("YES") : F("NO"));
}

void setFan(bool enabled) {
  if (enabled == fanOn) return;

  fanOn = enabled;

  if (enabled) {
    fanBoostUntil = millis() + FAN_START_BOOST_MS;
    writeFanPower(FAN_START_POWER, F("START BOOST"));
  } else {
    fanBoostUntil = 0;
    writeFanPower(0, F("OFF"));
  }

  logStateChanges();
}

void serviceFanBoost() {
  if (!fanOn) return;
  if (fanBoostUntil == 0) return;

  if (millis() >= fanBoostUntil) {
    fanBoostUntil = 0;
    writeFanPower(FAN_RUN_POWER, F("RUN POWER"));
  }
}

// ============================================================
// SENSORI / ATTUATORI
// ============================================================
void readSensors() {
  int t = 0;
  int h = 0;

  if (dht11.readTemperatureHumidity(t, h) == 0) {
    temperature = t;
    humidity = h;
    sensorOk = true;
  } else {
    sensorOk = false;
  }

  logStateChanges();
}

void setWindows(bool open) {
  if (open == windowMotorOpen) {
    windowsOpen = open;
    return;
  }

  windowMotor.step(open ? PASSI_SPORTELLO : -PASSI_SPORTELLO);

  windowMotorOpen = open;
  windowsOpen = open;
  logStateChanges();
}

void applyLocalClimateAutomation() {
  if (!sensorOk) {
    setFan(false);
    return;
  }

  if (!fanOn && temperature >= SOGLIA_VENTOLA_ON) {
    setFan(true);
  } else if (fanOn && temperature <= SOGLIA_VENTOLA_OFF) {
    setFan(false);
  }

  setWindows(temperature >= 24);
}

// ============================================================
// COMUNICAZIONE
// ============================================================
String parseKey(const String& line, const char* key) {
  String token = String(key) + "=";
  int start = line.indexOf(token);

  if (start < 0) return "";

  int valStart = start + token.length();
  int valEnd = line.indexOf(';', valStart);

  if (valEnd < 0) valEnd = line.length();

  return line.substring(valStart, valEnd);
}

bool hasKey(const String& line, const char* key) {
  String token = String(key) + "=";
  return line.indexOf(token) >= 0;
}

bool parseBoolValue(const String& v, bool oldValue) {
  if (v == "1" || v == "true" || v == "TRUE" || v == "on" || v == "ON") return true;
  if (v == "0" || v == "false" || v == "FALSE" || v == "off" || v == "OFF") return false;

  return oldValue;
}

void applyBoolKey(const String& line, const char* key, bool& target) {
  if (!hasKey(line, key)) return;

  String v = parseKey(line, key);
  target = parseBoolValue(v, target);
}

void applyIntKey(const String& line, const char* key, int& target, int minVal, int maxVal) {
  if (!hasKey(line, key)) return;

  String v = parseKey(line, key);
  target = constrain(v.toInt(), minVal, maxVal);
}

void handleIncomingCommands() {
  static String buffer;
  const uint16_t MAX_CMD_LEN = 240;

  while (espLink.available() > 0) {
    char c = (char)espLink.read();

    if (c == '\n') {
      buffer.trim();

      int commandStart = buffer.indexOf("CMD;");
      if (commandStart >= 0) {
        if (commandStart > 0) {
          buffer = buffer.substring(commandStart);
        }
        commandReceivedCount++;
        bool climateCommand = hasKey(buffer, "fanOn") || hasKey(buffer, "windowsOpen");

        bool nextFan = fanOn;
        bool nextWindows = windowsOpen;

        applyBoolKey(buffer, "fanOn", nextFan);
        applyBoolKey(buffer, "windowsOpen", nextWindows);

        applyBoolKey(buffer, "internalDoorUnlocked", internalDoorUnlocked);
        applyBoolKey(buffer, "lcdEnabled", lcdEnabled);
        applyBoolKey(buffer, "intrusionAlarmArmed", intrusionAlarmArmed);
        applyBoolKey(buffer, "rfidReaderEnabled", rfidReaderEnabled);
        applyBoolKey(buffer, "parkingBarrierOpen", parkingBarrierOpen);
        applyBoolKey(buffer, "vehicleDetected", vehicleDetected);
        applyBoolKey(buffer, "twilightDetected", twilightDetected);
        applyBoolKey(buffer, "exteriorLightsOn", exteriorLightsOn);
        applyBoolKey(buffer, "awningOpen", awningOpen);
        applyBoolKey(buffer, "motionDetected", motionDetected);
        applyBoolKey(buffer, "indoorLightsOn", indoorLightsOn);

        applyIntKey(buffer, "parkingCapacity", parkingCapacity, 0, 999);
        applyIntKey(buffer, "occupiedSpots", occupiedSpots, 0, parkingCapacity);

        setFan(nextFan);
        setWindows(nextWindows);

        lastAnyCommandReceived = millis();

        if (climateCommand) {
          lastClimateCommandReceived = millis();
        }

        if (hasKey(buffer, "playBuzzer")) {
          String melody = parseKey(buffer, "buzzerMelody");
          applyIntKey(buffer, "buzzerSpeed", currentBuzzerSpeed, 50, 200);
          bool buzzerEnabled = parseBoolValue(parseKey(buffer, "buzzerEnabled"), true);
          buzzerEnabled = parseBoolValue(parseKey(buffer, "doomBuzzerEnabled"), buzzerEnabled);
          if (!buzzerEnabled) {
            buzzerStopRequested = true;
            noTone(PIN_BUZZER);
          } else if (buzzerPlaybackActive) {
            // Ignore repeated play commands while the current melody is still running.
          } else if (melody == "doom") {
            playDoomBuzzer();
          } else if (melody == "toreador") {
            playToreadorMarch();
          } else if (melody == "mega") {
            playMegaBossPattern();
          } else if (melody == "siren118") {
            playItalianAmbulanceSiren();
          } else {
            playFnafMusicBox();
          }
        }

        if (DEBUG_VERBOSE || climateCommand || hasKey(buffer, "playBuzzer")) {
          Serial.print(F("[ESP32 CMD] "));
          Serial.println(buffer);
        }
      } else if (buffer.length() > 0) {
        ignoredCommandCount++;
        if (DEBUG_VERBOSE) {
          Serial.print(F("[ESP32 SKIP] "));
          Serial.println(buffer);
        }
      }

      buffer = "";
    } else if (c != '\r') {
      if (buffer.length() < MAX_CMD_LEN) {
        buffer += c;
      } else {
        buffer = "";
      }
    }
  }
}

void printStateTo(Stream& out) {
  out.print(F("STATE;"));
  out.print(F("temperature=")); out.print(temperature);
  out.print(F(";humidity=")); out.print(humidity);
  out.print(F(";sensorOk=")); out.print(sensorOk ? 1 : 0);
  out.print(F(";fanOn=")); out.print(fanOn ? 1 : 0);
  out.print(F(";fanPower=")); out.print(lastFanPower);
  out.print(F(";fanPwm=")); out.print(lastFanPwm);
  out.print(F(";windowsOpen=")); out.print(windowsOpen ? 1 : 0);
  out.print(F(";internalDoorUnlocked=")); out.print(internalDoorUnlocked ? 1 : 0);
  out.print(F(";lcdEnabled=")); out.print(lcdEnabled ? 1 : 0);
  out.print(F(";remoteOverride=")); out.print(remoteClimateOverrideActive() ? 1 : 0);
  out.print(F(";module=clima"));
  out.print(F(";board=mega"));
  out.print(F(";uptimeMs=")); out.print(millis());
  out.println(';');
}

void sendState() {
  printStateTo(espLink);
  stateSentCount++;

  if (DEBUG_VERBOSE) {
    Serial.print(F("[SERIAL1 TX] "));
    printStateTo(Serial);
  }
}

// ============================================================
// GRAFICA HUD CYBERPUNK ULTRA
// ============================================================
void drawLabel(int16_t x, int16_t y, int16_t w, const __FlashStringHelper* label) {
  display.fillRect(x, y, w, 8, SSD1306_WHITE);
  display.drawPixel(x, y, SSD1306_BLACK);
  display.drawPixel(x + w - 1, y + 7, SSD1306_BLACK);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(x + 2, y + 1);
  display.print(label);

  display.setTextColor(SSD1306_WHITE);
}

void drawHexFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
  display.drawLine(x + 4, y, x + w - 5, y, SSD1306_WHITE);
  display.drawLine(x + w - 1, y + 4, x + w - 1, y + h - 5, SSD1306_WHITE);
  display.drawLine(x + w - 5, y + h - 1, x + 4, y + h - 1, SSD1306_WHITE);
  display.drawLine(x, y + 4, x, y + h - 5, SSD1306_WHITE);

  display.drawLine(x + 4, y, x, y + 4, SSD1306_WHITE);
  display.drawLine(x + w - 5, y, x + w - 1, y + 4, SSD1306_WHITE);
  display.drawLine(x + w - 1, y + h - 5, x + w - 5, y + h - 1, SSD1306_WHITE);
  display.drawLine(x, y + h - 5, x + 4, y + h - 1, SSD1306_WHITE);
}

void drawSegmentedBar(int16_t x, int16_t y, int16_t w, int16_t h, int value, int minVal, int maxVal) {
  display.drawRect(x, y, w, h, SSD1306_WHITE);

  int usable = w - 2;
  int fillW = mapClamped(value, minVal, maxVal, 0, usable);

  for (int i = 0; i < fillW; i += 4) {
    int segW = min(2, fillW - i);
    display.fillRect(x + 1 + i, y + 1, segW, h - 2, SSD1306_WHITE);
  }

  int scanX = x + 1 + ((uiFrame * 2) % max(1, usable));

  if (scanX < x + 1 + fillW) {
    display.drawFastVLine(scanX, y, h, SSD1306_WHITE);
  }
}

void drawCyberRain() {
  for (uint8_t i = 0; i < 11; i++) {
    uint8_t x = (i * 17 + uiFrame * 3) & 127;
    uint8_t y = 11 + ((i * 13 + uiFrame * 2) % 42);

    display.drawPixel(x, y, SSD1306_WHITE);

    if ((i + uiFrame) & 1) {
      display.drawPixel(x, y + 1, SSD1306_WHITE);
    }

    if ((i + uiFrame) % 5 == 0) {
      display.drawFastVLine(x, y, 3, SSD1306_WHITE);
    }
  }
}

void drawScanlines() {
  for (uint8_t y = 12 + (uiFrame & 3); y < 53; y += 8) {
    display.drawFastHLine(0, y, 128, SSD1306_WHITE);
  }
}

void drawEdgeGlyphs() {
  if (!OLED_EXTREME_FX) return;

  for (uint8_t i = 0; i < 9; i++) {
    uint8_t y = 12 + i * 5;
    uint8_t phase = (uiFrame + i * 3) & 15;

    display.drawPixel(126, y, SSD1306_WHITE);
    display.drawFastHLine(121 - phase / 2, y + 1, 4 + (phase & 3), SSD1306_WHITE);

    if ((uiFrame + i) & 2) {
      display.drawPixel(1, y + 2, SSD1306_WHITE);
      display.drawFastHLine(2, y + 3, 3 + (phase & 1), SSD1306_WHITE);
    }
  }
}

void drawHeatHalo() {
  if (!OLED_EXTREME_FX) return;

  int strength = mapClamped(temperature, SOGLIA_VENTOLA_OFF, TEMP_MAX, 0, 5);
  if (strength <= 0) return;

  int cx = 31;
  int cy = 39;

  for (int r = 9; r <= 9 + strength * 3; r += 3) {
    int wobble = ((uiFrame + r) & 3) - 1;
    display.drawCircle(cx + wobble, cy, r, SSD1306_WHITE);
  }

  if (temperature >= 29 && (uiFrame & 1)) {
    display.drawFastHLine(2, 22 + (uiFrame % 24), 58, SSD1306_WHITE);
  }
}

void drawHumidityConstellation() {
  if (!OLED_EXTREME_FX) return;

  for (uint8_t i = 0; i < 6; i++) {
    int x = 70 + ((i * 9 + uiFrame) % 48);
    int y = 20 + ((i * 7 + uiFrame / 2) % 28);

    display.drawPixel(x, y, SSD1306_WHITE);
    if (i > 0 && ((uiFrame + i) & 3) == 0) {
      int px = 70 + (((i - 1) * 9 + uiFrame) % 48);
      int py = 20 + (((i - 1) * 7 + uiFrame / 2) % 28);
      display.drawLine(px, py, x, y, SSD1306_WHITE);
    }
  }
}

void drawFanShockwave() {
  if (!OLED_EXTREME_FX || !fanOn) return;

  int r = 4 + ((uiFrame * 2) % 18);
  display.drawCircle(119, 59, r, SSD1306_WHITE);

  if ((uiFrame & 3) == 0) {
    display.drawFastHLine(97, 57, 13, SSD1306_WHITE);
    display.drawFastHLine(99, 61, 10, SSD1306_WHITE);
  }
}

void drawSignalSpine() {
  if (!OLED_EXTREME_FX) return;

  int base = remoteClimateOverrideActive() ? 1 : 0;
  for (uint8_t i = 0; i < 8; i++) {
    int x = 12 + i * 6;
    int h = 1 + ((uiFrame + i * 2 + base * 5) & 7);
    display.drawFastVLine(x, 57 - h, h, SSD1306_WHITE);
  }
}

void drawMegaCircuitBackdrop() {
  if (!OLED_EXTREME_FX) return;

  uint8_t sweep = (uiFrame * 3) & 127;

  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);
  display.drawFastHLine(0, 58, 128, SSD1306_WHITE);
  display.drawFastVLine(63, 12, 46, SSD1306_WHITE);

  for (uint8_t i = 0; i < 7; i++) {
    uint8_t y = 15 + i * 6;
    uint8_t leftPulse = (sweep + i * 11) & 63;
    uint8_t rightPulse = 65 + ((sweep + i * 17) % 62);

    display.drawFastHLine(2, y, 18 + (i % 3) * 4, SSD1306_WHITE);
    display.drawPixel(leftPulse, y, SSD1306_WHITE);
    display.drawPixel(rightPulse, y + 2, SSD1306_WHITE);

    if ((uiFrame + i) & 1) {
      display.drawFastHLine(108 - (i % 2) * 8, y + 1, 18, SSD1306_WHITE);
    }
  }

  for (uint8_t x = (uiFrame & 7); x < 128; x += 16) {
    display.drawPixel(x, 12, SSD1306_WHITE);
    display.drawPixel(127 - x, 57, SSD1306_WHITE);
  }
}

void drawThermalCore(int16_t cx, int16_t cy) {
  if (!OLED_EXTREME_FX) return;

  int heat = mapClamped(temperature, SOGLIA_VENTOLA_OFF, TEMP_MAX, 1, 5);

  for (uint8_t r = 5; r <= 5 + heat * 3; r += 3) {
    int wobble = ((uiFrame + r) & 3) - 1;
    display.drawCircle(cx + wobble, cy, r, SSD1306_WHITE);
  }

  display.drawLine(cx - 13, cy, cx + 13, cy, SSD1306_WHITE);
  display.drawLine(cx, cy - 13, cx, cy + 13, SSD1306_WHITE);

  if (fanOn) {
    uint8_t blade = fanFrame & 3;
    if (blade == 0 || blade == 2) {
      display.drawLine(cx - 10, cy - 10, cx + 10, cy + 10, SSD1306_WHITE);
      display.drawLine(cx + 10, cy - 10, cx - 10, cy + 10, SSD1306_WHITE);
    } else {
      display.drawFastVLine(cx, cy - 14, 29, SSD1306_WHITE);
      display.drawFastHLine(cx - 14, cy, 29, SSD1306_WHITE);
    }
  }
}

void drawMegaStatusRail() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(2, 61);
  display.print(sensorOk ? F("DHT") : F("ERR"));

  display.setCursor(27, 61);
  display.print(remoteClimateOverrideActive() ? F("ESP") : F("AUTO"));

  display.setCursor(58, 61);
  display.print(fanOn ? F("FAN+") : F("FAN-"));

  display.setCursor(91, 61);
  display.print(windowsOpen ? F("WIN+") : F("WIN-"));

  if (lastAnyCommandReceived != 0 && millis() - lastAnyCommandReceived < RX_FLASH_MS) {
    display.fillRect(116, 59, 12, 5, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(117, 58);
    display.print(F("RX"));
    display.setTextColor(SSD1306_WHITE);
  }
}

void drawHeader() {
  bool remote = remoteClimateOverrideActive();
  bool rxFlash = lastAnyCommandReceived != 0 && millis() - lastAnyCommandReceived < RX_FLASH_MS;

  display.fillRect(0, 0, 92, 10, SSD1306_WHITE);
  display.fillTriangle(92, 0, 104, 0, 92, 10, SSD1306_BLACK);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  display.setCursor(2, 1);
  display.print(F("MEGA"));

  display.setCursor(39, 1);
  display.print(remote ? F("ESP") : F("AUTO"));

  display.setCursor(65, 1);
  display.print(F("V7"));

  display.setTextColor(SSD1306_WHITE);

  display.drawFastHLine(103, 2, 23, SSD1306_WHITE);
  display.drawFastHLine(103, 5, 17, SSD1306_WHITE);
  display.drawFastHLine(103, 8, 11, SSD1306_WHITE);

  uint8_t pulseX = 103 + ((uiFrame * 4) % 23);
  display.drawPixel(pulseX, 2 + ((uiFrame & 1) * 3), SSD1306_WHITE);

  if (rxFlash && (uiFrame & 1)) {
    display.fillRect(105, 0, 23, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(110, 1);
    display.print(F("RX"));
    display.setTextColor(SSD1306_WHITE);
  }

  // Glitch header
  if ((uiFrame % 37) < 5) {
    int gx = 15 + ((uiFrame * 5) % 43);
    display.fillRect(gx, 2, 15, 2, SSD1306_BLACK);
    display.drawFastHLine(gx + 3, 11, 22, SSD1306_WHITE);
  }
}

void drawDivider() {
  for (int y = 13; y < 53; y += 3) {
    int x = 62 + (((y + uiFrame) & 4) ? 1 : 0);
    display.drawPixel(x, y, SSD1306_WHITE);

    if ((y + uiFrame) & 1) {
      display.drawPixel(x + 2, y, SSD1306_WHITE);
    }
  }
}

void drawSparkline(int16_t x, int16_t y, int16_t w, int16_t h, int8_t* data, int minVal, int maxVal) {
  if (!historyReady) return;

  int prevX = x;
  int prevY = y + h / 2;

  for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
    uint8_t idx = (historyIndex + i) % HISTORY_SIZE;
    int v = data[idx];

    int px = x + map(i, 0, HISTORY_SIZE - 1, 0, w - 1);
    int py = y + h - 1 - mapClamped(v, minVal, maxVal, 0, h - 1);

    if (i > 0) {
      display.drawLine(prevX, prevY, px, py, SSD1306_WHITE);
    }

    prevX = px;
    prevY = py;
  }
}

void drawTempPanel() {
  drawLabel(2, 13, 47, F("THERM"));
  drawHeatHalo();
  drawThermalCore(31, 39);

  display.drawFastVLine(0, 23, 30, SSD1306_WHITE);
  display.drawFastHLine(2, 24, 56, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(4, 26);

  if (temperature >= 0 && temperature < 10) {
    display.print(F("0"));
  }

  display.print(temperature);

  display.setTextSize(1);
  display.setCursor(43, 27);
  display.print((char)247);
  display.setCursor(43, 36);
  display.print(F("C"));

  // Termometro verticale cyber
  display.drawRect(54, 17, 7, 35, SSD1306_WHITE);

  int fillH = mapClamped(temperature, TEMP_MIN, TEMP_MAX, 0, 31);

  if (fillH > 0) {
    display.fillRect(56, 49 - fillH, 3, fillH, SSD1306_WHITE);
  }

  int scanY = 49 - ((uiFrame * 2) % 31);
  display.drawFastHLine(53, scanY, 9, SSD1306_WHITE);

  // Sparkline temperatura
  display.drawFastHLine(3, 48, 48, SSD1306_WHITE);
  drawSparkline(3, 49, 48, 4, tempHistory, TEMP_MIN, TEMP_MAX);

  // Barra temperatura
  drawSegmentedBar(3, 54, 56, 4, temperature, TEMP_MIN, TEMP_MAX);

  // Warning caldo
  if (temperature >= SOGLIA_VENTOLA_ON && (uiFrame & 2)) {
    display.drawTriangle(48, 17, 58, 17, 53, 11, SSD1306_WHITE);
    display.drawPixel(53, 15, SSD1306_BLACK);
  }
}

void drawRadarSweep(int16_t cx, int16_t cy, int16_t r) {
  uint8_t phase = uiFrame & 7;

  switch (phase) {
    case 0: display.drawLine(cx, cy, cx + r, cy, SSD1306_WHITE); break;
    case 1: display.drawLine(cx, cy, cx + r - 3, cy - r + 3, SSD1306_WHITE); break;
    case 2: display.drawLine(cx, cy, cx, cy - r, SSD1306_WHITE); break;
    case 3: display.drawLine(cx, cy, cx - r + 3, cy - r + 3, SSD1306_WHITE); break;
    case 4: display.drawLine(cx, cy, cx - r, cy, SSD1306_WHITE); break;
    case 5: display.drawLine(cx, cy, cx - r + 3, cy + r - 3, SSD1306_WHITE); break;
    case 6: display.drawLine(cx, cy, cx, cy + r, SSD1306_WHITE); break;
    case 7: display.drawLine(cx, cy, cx + r - 3, cy + r - 3, SSD1306_WHITE); break;
  }
}

void drawHumidityPanel() {
  drawLabel(67, 13, 42, F("HUMID"));
  drawHumidityConstellation();

  drawHexFrame(65, 24, 62, 28);

  int cx = 96;
  int cy = 37;

  display.drawCircle(cx, cy, 13, SSD1306_WHITE);
  display.drawCircle(cx, cy, 7, SSD1306_WHITE);
  drawRadarSweep(cx, cy, 13);

  // Particelle umidità
  for (uint8_t i = 0; i < 5; i++) {
    int x = 69 + i * 11;
    int y = 25 + ((uiFrame + i * 7) % 21);

    display.drawPixel(x, y, SSD1306_WHITE);

    if ((uiFrame + i) & 1) {
      display.drawPixel(x, y + 1, SSD1306_WHITE);
    }
  }

  int hx;

  if (humidity >= 100) hx = 76;
  else if (humidity < 10) hx = 88;
  else hx = 82;

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(hx, 31);
  display.print(humidity);

  display.setTextSize(1);

  int percentX = hx + 25;
  if (humidity >= 100) percentX = hx + 37;
  if (humidity < 10) percentX = hx + 13;

  display.setCursor(percentX, 38);
  display.print(F("%"));

  drawSegmentedBar(70, 54, 52, 4, humidity, 0, 100);

  if (humidity >= 75 && (uiFrame & 4)) {
    display.drawFastHLine(116, 15, 8, SSD1306_WHITE);
    display.drawFastHLine(118, 17, 6, SSD1306_WHITE);
  }
}

void drawFanTurbine(int16_t cx, int16_t cy, bool active) {
  display.drawCircle(cx, cy, 5, SSD1306_WHITE);
  display.drawPixel(cx, cy, SSD1306_WHITE);

  if (!active) {
    display.drawFastHLine(cx - 2, cy, 5, SSD1306_WHITE);
    return;
  }

  switch (fanFrame & 3) {
    case 0:
      display.drawLine(cx - 4, cy - 4, cx + 4, cy + 4, SSD1306_WHITE);
      display.drawLine(cx + 4, cy - 4, cx - 4, cy + 4, SSD1306_WHITE);
      break;

    case 1:
      display.drawFastVLine(cx, cy - 5, 11, SSD1306_WHITE);
      display.drawFastHLine(cx - 5, cy, 11, SSD1306_WHITE);
      break;

    case 2:
      display.drawLine(cx - 5, cy - 1, cx + 5, cy + 1, SSD1306_WHITE);
      display.drawLine(cx + 1, cy - 5, cx - 1, cy + 5, SSD1306_WHITE);
      break;

    case 3:
      display.drawLine(cx - 1, cy - 5, cx + 1, cy + 5, SSD1306_WHITE);
      display.drawLine(cx - 5, cy + 1, cx + 5, cy - 1, SSD1306_WHITE);
      break;
  }
}

void drawFooter() {
  display.drawFastHLine(0, 59, 128, SSD1306_WHITE);
  drawSignalSpine();
  drawMegaStatusRail();

  display.drawRect(111, 54, 17, 10, SSD1306_WHITE);
  drawFanTurbine(119, 59, fanOn);
  drawFanShockwave();
}

void drawFaultScreen() {
  drawCyberRain();
  drawHeader();

  drawHexFrame(5, 16, 118, 39);

  if (uiFrame & 2) {
    display.fillRect(12, 21, 104, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }

  display.setTextSize(2);
  display.setCursor(14, 20);
  display.print(F("SYS FAULT"));

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18, 41);
  display.print(F("DHT LINK OFFLINE"));

  int scanX = 8 + ((uiFrame * 4) % 90);
  display.drawFastHLine(scanX, 52, 26, SSD1306_WHITE);
}

void drawPharmacyCross(int16_t cx, int16_t cy, uint8_t size, bool filled, uint16_t color) {
  int arm = size / 3;

  if (filled) {
    display.fillRect(cx - arm, cy - size, arm * 2 + 1, size * 2 + 1, color);
    display.fillRect(cx - size, cy - arm, size * 2 + 1, arm * 2 + 1, color);
  } else {
    display.drawRect(cx - arm, cy - size, arm * 2 + 1, size * 2 + 1, color);
    display.drawRect(cx - size, cy - arm, size * 2 + 1, arm * 2 + 1, color);
  }
}

void drawPharmacyTicker(const __FlashStringHelper* text) {
  int16_t offset = 128 - ((uiFrame * 2) % 210);

  display.drawFastHLine(0, 55, 128, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(offset, 57);
  display.print(text);
}

void drawPharmacyCrossScreen() {
  uint8_t pulse = (uiFrame / 3) & 7;
  bool invert = (uiFrame / 12) & 1;

  if (invert) {
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }

  for (uint8_t i = 0; i < 4; i++) {
    int x = 10 + i * 34;
    int y = 8 + ((uiFrame + i * 9) % 42);
    if (invert) display.drawPixel(x, y, SSD1306_BLACK);
    else display.drawPixel(x, y, SSD1306_WHITE);
  }

  drawPharmacyCross(64, 28, 18 + pulse, !invert, invert ? SSD1306_BLACK : SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(6, 5);
  display.print(F("CLIMA MEGA"));

  display.setCursor(86, 5);
  display.print(sensorOk ? F("APERTO") : F("DHT?"));

  display.setTextSize(2);
  display.setCursor(8, 41);
  display.print(temperature);
  display.print((char)247);
  display.print(F("C"));

  display.setCursor(76, 41);
  display.print(humidity);
  display.print(F("%"));

  if (invert) {
    display.setTextColor(SSD1306_WHITE);
  }
}

void drawPharmacyWaveScreen() {
  drawHeader();

  for (uint8_t y = 14; y < 52; y += 6) {
    for (uint8_t x = 0; x < 128; x += 6) {
      uint8_t phase = (x + y + uiFrame * 3) & 15;
      if (phase < 5) {
        display.drawPixel(x, y + phase / 2, SSD1306_WHITE);
      }
    }
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 17);
  display.print(F("TERMO FLOW"));

  display.setTextSize(3);
  display.setCursor(7, 28);
  display.print(temperature);

  display.setTextSize(1);
  display.setCursor(47, 30);
  display.print((char)247);
  display.setCursor(47, 39);
  display.print(F("C"));

  display.drawRect(67, 18, 56, 30, SSD1306_WHITE);
  drawSparkline(70, 22, 50, 20, tempHistory, TEMP_MIN, TEMP_MAX);
  drawSegmentedBar(70, 51, 51, 4, temperature, TEMP_MIN, TEMP_MAX);

  drawPharmacyTicker(F("VENTOLA AUTOMATICA  FINESTRE SMART  ESP32 ONLINE"));
}

void drawPharmacyFanScreen() {
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  display.drawRect(5, 5, 118, 54, SSD1306_WHITE);

  int cx = 35;
  int cy = 31;
  int r = 10 + ((uiFrame / 2) % 8);

  display.drawCircle(cx, cy, r, SSD1306_WHITE);
  display.drawCircle(cx, cy, r + 6, SSD1306_WHITE);
  drawFanTurbine(cx, cy, true);

  if (fanOn) {
    display.drawFastHLine(54, 21 + (uiFrame % 18), 62, SSD1306_WHITE);
    display.drawFastHLine(58, 25 + ((uiFrame * 2) % 14), 54, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(57, 13);
  display.print(F("VENTOLA"));

  display.setTextSize(2);
  display.setCursor(57, 26);
  display.print(fanOn ? F("ON") : F("OFF"));

  display.setTextSize(1);
  display.setCursor(57, 46);
  display.print(windowsOpen ? F("FINESTRE APERTE") : F("FINESTRE CHIUSE"));
}

void drawPharmacyShowcase() {
  uint8_t mode = (uiFrame / 145) % 4;

  if (mode == 1) {
    drawPharmacyCrossScreen();
  } else if (mode == 2) {
    drawPharmacyWaveScreen();
  } else if (mode == 3) {
    drawPharmacyFanScreen();
  } else {
    drawCyberRain();
    drawMegaCircuitBackdrop();
    drawEdgeGlyphs();

    if ((uiFrame & 7) == 0) {
      drawScanlines();
    }

    drawHeader();
    drawDivider();
    drawTempPanel();
    drawHumidityPanel();
    drawFooter();
  }
}

void drawReadableDashboard() {
  bool remote = remoteClimateOverrideActive();
  bool rxFlash = lastAnyCommandReceived != 0 && millis() - lastAnyCommandReceived < RX_FLASH_MS;
  bool alert = temperature >= SOGLIA_VENTOLA_ON || humidity >= 75 || windowsOpen;

  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 2);
  display.print(F("CLIMA ICU"));
  display.setCursor(62, 2);
  display.print(remote ? F("ESP32") : F("AUTO"));
  display.setCursor(101, 2);
  display.print(rxFlash ? F("RX") : (alert ? F("ALM") : F("OK")));

  display.setTextColor(SSD1306_WHITE);
  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);
  display.drawFastVLine(64, 11, 31, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(5, 15);
  display.print(F("TEMP C"));
  display.setCursor(70, 15);
  display.print(F("HUM %"));

  display.setTextSize(3);
  display.setCursor(5, 23);
  if (temperature >= 0 && temperature < 10) display.print(F("0"));
  display.print(temperature);
  display.setTextSize(1);
  display.setCursor(48, 25);
  display.print((char)247);

  display.setTextSize(3);
  display.setCursor(humidity >= 100 ? 70 : 80, 23);
  display.print(humidity);
  display.setTextSize(1);
  display.setCursor(humidity >= 100 ? 116 : 112, 32);
  display.print(F("%"));

  display.drawFastHLine(2, 43, 124, SSD1306_WHITE);

  // Traccia tipo monitor ospedaliero: calma in AUTO, piu' nervosa in allarme.
  int baseY = 51;
  int startX = (uiFrame * 2) & 7;
  for (int x = -startX; x < 128; x += 16) {
    int y = baseY;
    display.drawLine(x, y, x + 4, y, SSD1306_WHITE);
    display.drawLine(x + 4, y, x + 6, y - 3, SSD1306_WHITE);
    display.drawLine(x + 6, y - 3, x + 8, y + (alert ? 3 : 1), SSD1306_WHITE);
    display.drawLine(x + 8, y + (alert ? 3 : 1), x + 10, y - (fanOn ? 5 : 3), SSD1306_WHITE);
    display.drawLine(x + 10, y - (fanOn ? 5 : 3), x + 12, y, SSD1306_WHITE);
    display.drawLine(x + 12, y, x + 16, y, SSD1306_WHITE);
  }

  display.fillRect(0, 55, 128, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 56);
  display.print(fanOn ? F("FAN ON") : F("FAN OFF"));
  display.setCursor(48, 56);
  display.print(windowsOpen ? F("WIN OP") : F("WIN CL"));
  display.setCursor(96, 56);
  display.print(sensorOk ? F("DHT") : F("ERR"));
  display.setTextColor(SSD1306_WHITE);
}

void drawUI() {
  display.clearDisplay();

  if (!lcdEnabled) {
    display.display();
    return;
  }

  if (!sensorOk) {
    drawFaultScreen();
    display.display();
    return;
  }

  drawReadableDashboard();

  // Frame flash quando riceve dati
  if (lastAnyCommandReceived != 0 &&
      millis() - lastAnyCommandReceived < RX_FLASH_MS &&
      (uiFrame & 1)) {
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  }

  display.display();
}

// ============================================================
// SCHERMI DI TEST / BOOT
// ============================================================
void drawFanTestScreen(const __FlashStringHelper* label, uint8_t power) {
  display.clearDisplay();
  resetText();

  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 1);
  display.print(F("FAN DIAGNOSTIC"));

  display.setTextColor(SSD1306_WHITE);
  display.drawRect(6, 16, 116, 40, SSD1306_WHITE);

  display.setCursor(14, 22);
  display.print(F("MODE: "));
  display.print(label);

  display.setCursor(14, 34);
  display.print(F("POWER: "));
  display.print(power);

  display.setCursor(14, 46);
  display.print(F("PWM: "));
  display.print(fanPowerToPwm(power));

  display.setCursor(74, 46);
  if (FAN_ACTIVE_LOW) {
    display.print(F("LOW"));
  } else {
    display.print(F("NORM"));
  }

  drawFanTurbine(106, 34, power > 0);

  display.display();
}

void fanSelfTest() {
  if (!FAN_SELF_TEST_ON_BOOT) return;

  Serial.println(F("========== FAN SELF TEST =========="));
  Serial.println(F("Se non gira su FULL, prova FAN_ACTIVE_LOW=true oppure controlla alimentazione/GND/MOSFET."));

  drawFanTestScreen(F("OFF"), 0);
  writeFanPower(0, F("SELFTEST OFF"));
  delay(700);

  drawFanTestScreen(F("FULL"), 255);
  writeFanPower(255, F("SELFTEST FULL"));
  tone(PIN_BUZZER, 1800, 80);
  delay(1400);

  drawFanTestScreen(F("RUN"), FAN_RUN_POWER);
  writeFanPower(FAN_RUN_POWER, F("SELFTEST RUN"));
  tone(PIN_BUZZER, 2600, 80);
  delay(1200);

  writeFanPower(0, F("SELFTEST END OFF"));
  fanOn = false;
  fanBoostUntil = 0;

  Serial.println(F("========== END FAN SELF TEST =========="));
}

void bootCyberpunkIntro() {
  display.clearDisplay();
  resetText();

  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 2);
  display.print(F("CLIMA READY"));
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 19);
  display.print(F("OLED OK"));
  display.setCursor(2, 31);
  display.print(F("DHT11 PIN "));
  display.print(PIN_DHT);
  display.setCursor(2, 43);
  display.print(F("ESP32 SERIAL1"));
  display.display();
  delay(650);
  resetText();
  return;

  for (uint8_t f = 0; f < 30; f++) {
    display.clearDisplay();

    display.fillRect(0, 0, 128, 9, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(2, 1);
    display.print(F("NEXUS_BOOT // CYBERDECK"));

    display.setTextColor(SSD1306_WHITE);

    for (int y = 63; y >= 38; y -= 5) {
      display.drawFastHLine(0, y, 128, SSD1306_WHITE);
    }

    for (int x = -64; x < 192; x += 16) {
      display.drawLine(64, 38, x + ((f * 3) % 16), 63, SSD1306_WHITE);
    }

    for (uint8_t i = 0; i < 13; i++) {
      int x = (i * 19 + f * 5) & 127;
      int y = 12 + ((i * 11 + f * 2) % 22);

      display.drawPixel(x, y, SSD1306_WHITE);

      if ((i + f) & 1) {
        display.drawPixel(x, y + 1, SSD1306_WHITE);
      }
    }

    display.setCursor(8, 15);
    display.print(F("AUTH "));
    display.print(map(f, 0, 29, 0, 100));
    display.print(F("%"));

    display.drawRect(8, 28, 112, 7, SSD1306_WHITE);
    display.fillRect(10, 30, map(f, 0, 29, 0, 108), 3, SSD1306_WHITE);

    display.display();

    tone(PIN_BUZZER, 1100 + f * 70, 8);
    delay(32);
  }

  display.clearDisplay();
  resetText();

  display.setCursor(0, 5);
  display.println(F("> CORE HUD: ONLINE"));
  display.display();
  tone(PIN_BUZZER, 1500, 45);
  delay(230);

  display.println(F("> OLED BUS: OVERCLOCK"));
  display.display();
  tone(PIN_BUZZER, 1900, 45);
  delay(230);

  display.println(F("> DHT LINK: SYNC"));
  display.display();
  tone(PIN_BUZZER, 2300, 45);
  delay(230);

  display.println(F("> ESP BRIDGE: SERIAL1"));
  display.display();
  tone(PIN_BUZZER, 2700, 45);
  delay(230);

  display.println(F("> FAN DRIVER: DIAG"));
  display.display();
  tone(PIN_BUZZER, 3100, 45);
  delay(230);

  display.drawRect(9, 48, 110, 9, SSD1306_WHITE);

  for (int i = 0; i <= 106; i += 5) {
    display.fillRect(11, 50, i, 5, SSD1306_WHITE);

    if ((i / 5) & 1) {
      display.drawFastVLine(11 + i, 48, 9, SSD1306_BLACK);
    }

    display.display();
    tone(PIN_BUZZER, 3000 + i * 14, 7);
    delay(20);
  }

  display.clearDisplay();
  display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);

  display.setCursor(24, 20);
  display.print(F("NEXUS ONLINE"));

  display.setCursor(28, 34);
  display.print(F("MEGA HUD V7"));

  display.display();

  tone(PIN_BUZZER, 5200, 130);
  delay(330);

  resetText();
}

// ============================================================
// SETUP / LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  espLink.begin(57600);
  delay(200);
  logTag(F("BOOT"));
  Serial.println(F("Mega Serial1: TX1 pin 18 -> ESP32 RX16, RX1 pin 19 <- ESP32 TX17, baud 57600"));

  Wire.begin();
  Wire.setClock(400000);

  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  writeFanPower(0, F("BOOT INIT OFF"));
  noTone(PIN_BUZZER);

  windowMotor.setSpeed(2);

  randomSeed(micros());

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED non trovato!"));
  }

  display.setTextWrap(false);

  bootCyberpunkIntro();
  fanSelfTest();
  logTag(F("BOOT"));
  Serial.println(F("display pronto, test iniziale disattivato"));

  readSensors();

  if (sensorOk) {
    initHistory();
  }

  applyLocalClimateAutomation();
  drawUI();

  unsigned long now = millis();
  lastSensorRead = now;
  lastStateSend = now;
  lastUiAnim = now;
  lastDebugStatus = now;
  logStatusSummary();
}

void loop() {
  unsigned long now = millis();

  handleIncomingCommands();
  serviceFanBoost();

  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    readSensors();

    if (sensorOk) {
      pushHistory();
    }

    if (!remoteClimateOverrideActive()) {
      applyLocalClimateAutomation();
    }

    lastSensorRead = now;
  }

  if (now - lastStateSend >= STATE_INTERVAL_MS) {
    sendState();
    lastStateSend = now;
  }

  if (now - lastDebugStatus >= DEBUG_STATUS_MS) {
    logStatusSummary();
    lastDebugStatus = now;
  }

  if (now - lastUiAnim >= UI_ANIM_MS) {
    uiFrame++;

    if (fanOn) {
      fanFrame++;
    } else if ((uiFrame & 7) == 0) {
      fanFrame++;
    }

    drawUI();
    lastUiAnim = now;
  }
}
