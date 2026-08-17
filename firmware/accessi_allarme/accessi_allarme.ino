#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>
#include <LiquidCrystal.h>
#include "secrets.h"

// ============================================================
// WIFI / SERVER PHP
// ============================================================
const char* WIFI_SSID = "NOME_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";
const char* SERVER_BASE_URL = "https://TUO_DOMINIO.altervista.org/smart-controller";

const unsigned long SERVER_PULL_MS = 1000UL;
const unsigned long SERVER_PUSH_MS = 1000UL;
const unsigned long WIFI_RETRY_MS = 5000UL;
const unsigned long STATUS_LOG_MS = 15000UL;
const unsigned long DOOR_UNLOCK_MS = 2500UL;
const uint8_t MAX_DENIED_ATTEMPTS = 5;
const bool DEBUG_VERBOSE = false;
const bool BUZZER_ENABLED = true;

// ============================================================
// RFID / LED / BUZZER
// ============================================================
#define SS 5
#define RST 22
#define VERDE 32
#define ROSSO 33

const uint8_t PIN_BUZZER = 26;
MFRC522 rfid(SS, RST);
byte uidAutorizzato[4] = {0xE3, 0x8B, 0x49, 0x1A};

// ============================================================
// STEPPER PORTA
// ============================================================
const int PASSI_PER_ROTAZIONE = 2048;
const int PASSI_PORTA = 512;
const uint8_t STEPPER_IN1 = 13;
const uint8_t STEPPER_IN2 = 14;
const uint8_t STEPPER_IN3 = 12;
const uint8_t STEPPER_IN4 = 27;
const uint8_t STEPPER_RPM = 6;
// 28BYJ-48 + ULN2003: Arduino Stepper vuole IN1, IN3, IN2, IN4.
Stepper stepper(PASSI_PER_ROTAZIONE, STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);

// ============================================================
// LCD 16x2
// ============================================================
#define RS 21
#define EN 17
#define P4 16
#define P5 4
#define P6 2
#define P7 15
LiquidCrystal display(RS, EN, P4, P5, P6, P7);

// ============================================================
// STATO
// ============================================================
bool internalDoorUnlocked = false;
bool intrusionAlarmArmed = true;
bool rfidReaderEnabled = true;
bool doorMotorOpen = false;

uint8_t deniedAttempts = 0;
unsigned long doorUnlockedUntil = 0;
unsigned long lastServerPull = 0;
unsigned long lastServerPush = 0;
unsigned long lastWifiRetry = 0;
unsigned long lastStatusLog = 0;
int lastPullStatus = 0;
int lastPushStatus = 0;
unsigned long pullOkCount = 0;
unsigned long pullFailCount = 0;
unsigned long pushOkCount = 0;
unsigned long pushFailCount = 0;
unsigned long remoteChangeCount = 0;
unsigned long cardReadCount = 0;
unsigned long cardAllowCount = 0;
unsigned long cardDenyCount = 0;
bool remoteSecurityReady = false;

// ============================================================
// UTILITY
// ============================================================
String buildUrl(const char* path) {
  String url = SERVER_BASE_URL;
  url += path;
  return url;
}

bool useHttps() {
  return String(SERVER_BASE_URL).startsWith("https://");
}

void logLine(const __FlashStringHelper* area, const __FlashStringHelper* level, const String& message) {
  Serial.print('[');
  Serial.print(millis());
  Serial.print(F(" ms] "));
  Serial.print(area);
  Serial.print(' ');
  Serial.print(level);
  Serial.print(F(" | "));
  Serial.println(message);
}

bool validateRemoteSecurity() {
  if (String(API_TOKEN).length() < 32) {
    logLine(F("SECURITY"), F("ERR"), F("API_TOKEN mancante o troppo corto; rete remota disabilitata"));
    return false;
  }
  if (!useHttps()) {
    logLine(F("SECURITY"), F("ERR"), F("SERVER_BASE_URL deve usare HTTPS"));
    return false;
  }
  String ca = TLS_ROOT_CA;
  if (ca.length() < 100 || ca.indexOf("BEGIN CERTIFICATE") < 0) {
    logLine(F("SECURITY"), F("ERR"), F("TLS_ROOT_CA non configurata; rete remota disabilitata"));
    return false;
  }
  logLine(F("SECURITY"), F("OK"), F("Bearer token e verifica TLS configurati"));
  return true;
}

void addApiAuthorization(HTTPClient& http) {
  http.addHeader("Authorization", "Bearer " + String(API_TOKEN));
}

String boolText(bool value) {
  return value ? "1" : "0";
}

String stateSummary() {
  return "door=" + boolText(internalDoorUnlocked) +
         " rfid=" + boolText(rfidReaderEnabled) +
         " alarm=" + boolText(intrusionAlarmArmed) +
         " motor=" + boolText(doorMotorOpen) +
         " denied=" + String(deniedAttempts);
}

void logDebug(const __FlashStringHelper* area, const String& message) {
  if (!DEBUG_VERBOSE) return;
  logLine(area, F("DBG"), message);
}

void logPinMap() {
  logLine(F("BOOT"), F("PINS"), F("RFID SS=5 RST=22 SCK=18 MISO=19 MOSI=23"));
  logLine(F("BOOT"), F("PINS"), F("LED verde=32 rosso=33 buzzer=26"));
  logLine(F("BOOT"), F("PINS"), F("Stepper IN1=13 IN2=14 IN3=12 IN4=27 order=IN1,IN3,IN2,IN4"));
  logLine(F("BOOT"), F("PINS"), F("LCD RS=21 E=17 D4=16 D5=4 D6=2 D7=15"));
}

void showMessage(const __FlashStringHelper* line1, const __FlashStringHelper* line2) {
  display.clear();
  display.setCursor(0, 0);
  display.print(line1);
  display.setCursor(0, 1);
  display.print(line2);
}

void showIdleScreen() {
  display.clear();
  display.setCursor(0, 0);
  display.print(rfidReaderEnabled ? F("Aspetto chiave") : F("RFID disattivo"));
  display.setCursor(0, 1);
  display.print(intrusionAlarmArmed ? F("Allarme ARMATO") : F("Allarme OFF"));
}

void beepOk() {
  if (!BUZZER_ENABLED) return;
  tone(PIN_BUZZER, 880, 70);
  delay(100);
  tone(PIN_BUZZER, 1175, 80);
  delay(120);
  noTone(PIN_BUZZER);
}

void beepDenied() {
  if (!BUZZER_ENABLED) return;
  tone(PIN_BUZZER, 420, 120);
  delay(150);
  noTone(PIN_BUZZER);
}

void beepAlarm() {
  if (!BUZZER_ENABLED) return;
  for (int i = 0; i < 3; i++) {
    tone(PIN_BUZZER, 520, 90);
    delay(130);
    tone(PIN_BUZZER, 390, 90);
    delay(170);
  }
  noTone(PIN_BUZZER);
}

void beepNuclearAlarm() {
  if (!BUZZER_ENABLED) return;

  // Federal Signal 2T22/3T22-style attack wail. The real siren is dual-rotor;
  // a piezo can only play one tone, so alternate the 10/12-port tones quickly.
  for (uint8_t cycle = 0; cycle < 4; cycle++) {
    for (uint16_t low = 360; low <= 980; low += 16) {
      const uint16_t high = low * 6 / 5;
      tone(PIN_BUZZER, low, 14);
      delay(12);
      tone(PIN_BUZZER, high, 14);
      delay(12);
    }
    for (uint16_t low = 980; low >= 360; low -= 16) {
      const uint16_t high = low * 6 / 5;
      tone(PIN_BUZZER, low, 14);
      delay(12);
      tone(PIN_BUZZER, high, 14);
      delay(12);
    }
  }

  noTone(PIN_BUZZER);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  logLine(F("WIFI"), F("INFO"), F("connessione"));
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 10000UL) {
    delay(400);
    if (DEBUG_VERBOSE) Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (DEBUG_VERBOSE) Serial.println();
    logLine(F("WIFI"), F("OK"), "ip=" + WiFi.localIP().toString() + " rssi=" + String(WiFi.RSSI()) + "dBm");
  } else {
    if (DEBUG_VERBOSE) Serial.println();
    logLine(F("WIFI"), F("ERR"), F("non connesso"));
  }
}

void setDoorUnlocked(bool unlocked, bool moveMotor) {
  if (internalDoorUnlocked == unlocked && doorMotorOpen == unlocked) return;

  internalDoorUnlocked = unlocked;

  if (moveMotor && doorMotorOpen != unlocked) {
    stepper.step(unlocked ? PASSI_PORTA : -PASSI_PORTA);
    doorMotorOpen = unlocked;
  }

  digitalWrite(VERDE, unlocked ? HIGH : LOW);
  logLine(F("DOOR"), F("STATE"), unlocked ? F("sbloccata") : F("chiusa"));
}

bool uidMatchesAuthorizedCard() {
  if (rfid.uid.size < 4) return false;

  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != uidAutorizzato[i]) return false;
  }

  return true;
}

void printUid() {
  Serial.print(F("[RFID UID]"));
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? F(" 0") : F(" "));
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
}

void handleAuthorizedCard() {
  cardAllowCount++;
  deniedAttempts = 0;
  showMessage(F("Chiave OK"), F("ACCESSO PERMESSO"));
  digitalWrite(VERDE, HIGH);
  digitalWrite(ROSSO, LOW);
  beepOk();

  setDoorUnlocked(true, true);
  doorUnlockedUntil = millis() + DOOR_UNLOCK_MS;
}

void handleDeniedCard() {
  cardDenyCount++;
  deniedAttempts++;
  showMessage(F("Chiave letta"), F("ACCESSO ERRATO"));
  digitalWrite(ROSSO, HIGH);
  digitalWrite(VERDE, LOW);

  if (deniedAttempts <= 3) {
    beepDenied();
  } else if (deniedAttempts <= MAX_DENIED_ATTEMPTS) {
    beepAlarm();
  } else {
    logLine(F("ALARM"), F("NUCLEAR"), "troppi accessi negati=" + String(deniedAttempts));
    beepNuclearAlarm();
    deniedAttempts = 0;
  }

  digitalWrite(ROSSO, LOW);
  logLine(F("RFID"), F("DENY"), stateSummary());
  delay(450);
  showIdleScreen();
}

void readRfid() {
  if (!rfidReaderEnabled) return;
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  cardReadCount++;
  printUid();

  if (uidMatchesAuthorizedCard()) {
    logLine(F("RFID"), F("ALLOW"), F("chiave autorizzata"));
    handleAuthorizedCard();
  } else {
    handleDeniedCard();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void applyRemoteState(JsonObject state) {
  if (state.isNull()) {
    logLine(F("HTTP"), F("ERR"), F("PULL senza oggetto state"));
    return;
  }

  bool nextRfid = state["rfidReaderEnabled"] | rfidReaderEnabled;
  bool nextAlarm = state["intrusionAlarmArmed"] | intrusionAlarmArmed;
  bool changed = false;

  if (nextRfid != rfidReaderEnabled) {
    rfidReaderEnabled = nextRfid;
    changed = true;
    logLine(F("REMOTE"), F("STATE"), "rfidReaderEnabled=" + boolText(rfidReaderEnabled));
  }

  if (nextAlarm != intrusionAlarmArmed) {
    intrusionAlarmArmed = nextAlarm;
    changed = true;
    logLine(F("REMOTE"), F("STATE"), "intrusionAlarmArmed=" + boolText(intrusionAlarmArmed));
  }

  bool remoteDoor = state["internalDoorUnlocked"] | internalDoorUnlocked;
  if (remoteDoor != internalDoorUnlocked) {
    setDoorUnlocked(remoteDoor, true);
    doorUnlockedUntil = remoteDoor ? millis() + DOOR_UNLOCK_MS : 0;
    changed = true;
    logLine(F("REMOTE"), F("STATE"), "internalDoorUnlocked=" + boolText(internalDoorUnlocked));
  }

  if (changed) {
    remoteChangeCount++;
    showIdleScreen();
  }
}

bool pullRemoteStateHttp(HTTPClient& http) {
  addApiAuthorization(http);
  int status = http.GET();
  lastPullStatus = status;

  if (status != HTTP_CODE_OK) {
    pullFailCount++;
    logLine(F("HTTP"), F("ERR"), "PULL status=" + String(status));
    return false;
  }

  DynamicJsonDocument doc(2048);
  String body = http.getString();
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    pullFailCount++;
    logLine(F("HTTP"), F("ERR"), "JSON non valido: " + String(error.c_str()));
    return false;
  }

  applyRemoteState(doc["state"]);
  pullOkCount++;
  logDebug(F("HTTP"), "PULL body=" + body + " " + stateSummary());
  return true;
}

bool pullRemoteState() {
  if (WiFi.status() != WL_CONNECTED) {
    pullFailCount++;
    return false;
  }
  if (!remoteSecurityReady) {
    pullFailCount++;
    return false;
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setCACert(TLS_ROOT_CA);
  http.begin(client, buildUrl("/api/state.php"));
  bool ok = pullRemoteStateHttp(http);
  http.end();
  return ok;
}

void writeStatePayload(JsonDocument& doc) {
  doc["internalDoorUnlocked"] = internalDoorUnlocked;
  doc["intrusionAlarmArmed"] = intrusionAlarmArmed;
  doc["rfidReaderEnabled"] = rfidReaderEnabled;
}

bool pushDeviceStateHttp(HTTPClient& http, const String& body) {
  addApiAuthorization(http);
  http.addHeader("Content-Type", "application/json");
  int status = http.POST(body);
  lastPushStatus = status;

  if (status < 200 || status >= 300) {
    pushFailCount++;
    logLine(F("HTTP"), F("ERR"), "PUSH status=" + String(status) + " payload=" + body);
  } else {
    pushOkCount++;
    logDebug(F("HTTP"), "PUSH payload=" + body + " " + stateSummary());
  }

  return status >= 200 && status < 300;
}

bool pushDeviceState() {
  if (WiFi.status() != WL_CONNECTED) {
    pushFailCount++;
    return false;
  }
  if (!remoteSecurityReady) {
    pushFailCount++;
    return false;
  }

  DynamicJsonDocument doc(512);
  writeStatePayload(doc);

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  WiFiClientSecure client;
  client.setCACert(TLS_ROOT_CA);
  http.begin(client, buildUrl("/api/device_state.php"));
  bool ok = pushDeviceStateHttp(http, body);
  http.end();
  return ok;
}

void logStatusSummary() {
  String message = "wifi=" + String(WiFi.status() == WL_CONNECTED ? "ok" : "down") +
                   " security=" + String(remoteSecurityReady ? "ok" : "blocked") +
                   " rssi=" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + "dBm" +
                   " pull=" + String(pullOkCount) + "/" + String(pullFailCount) +
                   " push=" + String(pushOkCount) + "/" + String(pushFailCount) +
                   " lastPullHttp=" + String(lastPullStatus) +
                   " lastPushHttp=" + String(lastPushStatus) +
                   " remoteChanges=" + String(remoteChangeCount) +
                   " cards=" + String(cardReadCount) +
                   " allow=" + String(cardAllowCount) +
                   " deny=" + String(cardDenyCount) +
                   " " + stateSummary();
  logLine(F("STATUS"), F("INFO"), message);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  logLine(F("BOOT"), F("INFO"), F("accessi_allarme avvio"));
  logPinMap();

  SPI.begin();
  rfid.PCD_Init();

  display.begin(16, 2);
  showMessage(F("Porta interna"), F("Avvio..."));

  pinMode(VERDE, OUTPUT);
  pinMode(ROSSO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(VERDE, LOW);
  digitalWrite(ROSSO, LOW);
  noTone(PIN_BUZZER);

  stepper.setSpeed(STEPPER_RPM);
  connectWiFi();
  remoteSecurityReady = validateRemoteSecurity();
  showIdleScreen();

  unsigned long now = millis();
  lastServerPull = now;
  lastServerPush = now;
  lastWifiRetry = now;
  lastStatusLog = now;

  logLine(F("BOOT"), F("INFO"), F("accessi_allarme pronto"));
  logStatusSummary();
}

void loop() {
  unsigned long now = millis();

  readRfid();

  if (internalDoorUnlocked && doorUnlockedUntil != 0 && now >= doorUnlockedUntil) {
    setDoorUnlocked(false, true);
    doorUnlockedUntil = 0;
    showIdleScreen();
  }

  if (WiFi.status() != WL_CONNECTED && now - lastWifiRetry >= WIFI_RETRY_MS) {
    connectWiFi();
    lastWifiRetry = now;
  }

  if (now - lastServerPull >= SERVER_PULL_MS) {
    pullRemoteState();
    lastServerPull = now;
  }

  if (now - lastServerPush >= SERVER_PUSH_MS) {
    pushDeviceState();
    lastServerPush = now;
  }

  if (now - lastStatusLog >= STATUS_LOG_MS) {
    logStatusSummary();
    lastStatusLog = now;
  }
}
