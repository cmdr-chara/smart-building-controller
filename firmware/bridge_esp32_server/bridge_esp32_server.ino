#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============================================================
// CONFIGURAZIONE WIFI E SERVER
// ============================================================
const char* WIFI_SSID = "NOME_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";
const char* SERVER_BASE_URL = "https://TUO_DOMINIO.altervista.org/smart-controller";

// ============================================================
// CONFIGURAZIONE SERIALE (Comunicazione con Arduino Uno o Mega)
// ============================================================
// Questi sono GPIO dell'ESP32, non i pin dell'Arduino:
// ESP32 GPIO16 (RX2) <- TX Arduino/Mega (tramite partitore)
// ESP32 GPIO17 (TX2) -> RX Arduino/Mega
const uint8_t ESP_RX_PIN = 16;
const uint8_t ESP_TX_PIN = 17;
const unsigned long BAUD_RATES[] = {9600UL, 57600UL};
const uint8_t BAUD_RATE_COUNT = sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]);
const unsigned long BAUD_SCAN_MS = 5000UL;
const bool DEBUG_VERBOSE = false;

// ============================================================
// TEMPISTICHE
// ============================================================
const unsigned long INTERVALLO_PULL = 500UL;   // Comandi reattivi senza stressare troppo Altervista
const unsigned long INTERVALLO_PUSH = 5000UL;  // Push piu' lento per non bloccare i comandi
const unsigned long STATUS_LOG_MS = 15000UL;

// ============================================================
// STATO DEL SISTEMA
// ============================================================
struct DeviceState {
  float temperature = 0.0;
  int humidity = 0;
  bool sensorOk = true;
  bool fanOn = false;
  int fanPower = 0;
  int fanPwm = 0;
  bool windowsOpen = false;
  bool internalDoorUnlocked = false;
  bool intrusionAlarmArmed = true;
  bool rfidReaderEnabled = true;
  bool lcdEnabled = true;
  bool remoteOverride = false;
  int buzzerRequestId = 0;
  String buzzerMelody = "musicBox";
  int buzzerSpeed = 100;
  bool buzzerEnabled = false;
  bool doomBuzzerEnabled = false;
  int occupiedSpots = 0;
  int parkingCapacity = 32;
  bool parkingBarrierOpen = false;
  bool vehicleDetected = false;
  bool twilightDetected = false;
  bool exteriorLightsOn = false;
  bool awningOpen = true;
  bool motionDetected = false;
  bool indoorLightsOn = false;
};

DeviceState arduinoCurrent;  // Dati che arrivano dall'Arduino
DeviceState remoteDesired;   // Dati che arrivano dal Server

struct DesiredFieldFlags {
  bool fanOn = false;
  bool windowsOpen = false;
  bool lcdEnabled = false;
  bool internalDoorUnlocked = false;
  bool intrusionAlarmArmed = false;
  bool rfidReaderEnabled = false;
  bool parkingBarrierOpen = false;
  bool parkingCapacity = false;
  bool occupiedSpots = false;
  bool exteriorLightsOn = false;
  bool awningOpen = false;
  bool indoorLightsOn = false;
  bool buzzerMelody = false;
  bool buzzerSpeed = false;
  bool buzzerEnabled = false;
  bool doomBuzzerEnabled = false;
};

DesiredFieldFlags remoteHas;

String serialBuffer = "";
unsigned long lastPull = 0;
unsigned long lastPush = 0;
unsigned long lastBaudScan = 0;
unsigned long lastStatusLog = 0;
bool hasArduinoData = false;
bool serialByteSeen = false;
bool serialBaudLocked = false;
uint8_t currentBaudIndex = 0;
int lastForwardedBuzzerRequestId = 0;
bool commandSnapshotInitialized = false;
DeviceState lastCommandSnapshot;
int lastPullHttp = 0;
int lastPushHttp = 0;
unsigned long serialStateCount = 0;
unsigned long ignoredLineCount = 0;
unsigned long sentCommandCount = 0;
unsigned long pullOkCount = 0;
unsigned long pullFailCount = 0;
unsigned long pushOkCount = 0;
unsigned long pushFailCount = 0;

// ============================================================
// FUNZIONI UTILI
// ============================================================

void log(String tag, String msg) {
  Serial.printf("[%lu ms] [%s] %s\n", millis(), tag.c_str(), msg.c_str());
}

void logStatusSummary() {
  String msg = "wifi=" + String(WiFi.status() == WL_CONNECTED ? "ok" : "down");
  msg += " arduino=" + String(hasArduinoData ? "ok" : (serialByteSeen ? "noise" : "waiting"));
  msg += " baud=" + String(BAUD_RATES[currentBaudIndex]);
  msg += serialBaudLocked ? " locked" : " scanning";
  msg += " state=" + String(serialStateCount);
  msg += " ignored=" + String(ignoredLineCount);
  msg += " cmd=" + String(sentCommandCount);
  msg += " pull=" + String(pullOkCount) + "/" + String(pullFailCount);
  msg += " push=" + String(pushOkCount) + "/" + String(pushFailCount);
  msg += " lastHttp=" + String(lastPullHttp) + "/" + String(lastPushHttp);
  log("STATUS", msg);
}

// Estrae un valore dalla stringa seriale (es: "temp=25;")
String extractValue(String data, String key) {
  int start = data.indexOf(key + "=");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(";", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

bool extractBool(String data, String key, bool currentValue) {
  String value = extractValue(data, key);
  if (value == "") return currentValue;
  return value == "1" || value == "true" || value == "TRUE";
}

int extractInt(String data, String key, int currentValue) {
  String value = extractValue(data, key);
  if (value == "") return currentValue;
  return value.toInt();
}

float extractFloat(String data, String key, float currentValue) {
  String value = extractValue(data, key);
  if (value == "") return currentValue;
  return value.toFloat();
}

String printablePreview(String data) {
  String preview = "";
  int limit = min((int)data.length(), 80);

  for (int i = 0; i < limit; i++) {
    char c = data.charAt(i);
    preview += (c >= 32 && c <= 126) ? c : '.';
  }

  if (data.length() > limit) {
    preview += "...";
  }

  return preview;
}

// ============================================================
// PARSING SERIALE CON FILTRO ANTI-RUMORE
// ============================================================
void parseArduinoMessage(String line) {
  // FILTRO: Cerca la parola chiave STATE; ignorando lo sporco iniziale dei motori
  int pos = line.indexOf("STATE;");
  if (pos == -1) {
    if (line.length() > 0) {
      ignoredLineCount++;
      if (DEBUG_VERBOSE) {
        log("SERIAL", "Riga ignorata da Arduino: " + line);
      }
    }
    return;
  }

  String clean = line.substring(pos);  // Taglia via i simboli ??? iniziali

  // Aggiorna lo stato locale
  arduinoCurrent.temperature = extractFloat(clean, "temperature", arduinoCurrent.temperature);
  arduinoCurrent.humidity = extractInt(clean, "humidity", arduinoCurrent.humidity);
  arduinoCurrent.sensorOk = extractBool(clean, "sensorOk", arduinoCurrent.sensorOk);
  arduinoCurrent.fanOn = extractBool(clean, "fanOn", arduinoCurrent.fanOn);
  arduinoCurrent.fanPower = extractInt(clean, "fanPower", arduinoCurrent.fanPower);
  arduinoCurrent.fanPwm = extractInt(clean, "fanPwm", arduinoCurrent.fanPwm);
  arduinoCurrent.windowsOpen = extractBool(clean, "windowsOpen", arduinoCurrent.windowsOpen);
  arduinoCurrent.internalDoorUnlocked = extractBool(clean, "internalDoorUnlocked", arduinoCurrent.internalDoorUnlocked);
  arduinoCurrent.intrusionAlarmArmed = extractBool(clean, "intrusionAlarmArmed", arduinoCurrent.intrusionAlarmArmed);
  arduinoCurrent.rfidReaderEnabled = extractBool(clean, "rfidReaderEnabled", arduinoCurrent.rfidReaderEnabled);
  arduinoCurrent.lcdEnabled = extractBool(clean, "lcdEnabled", arduinoCurrent.lcdEnabled);
  arduinoCurrent.remoteOverride = extractBool(clean, "remoteOverride", arduinoCurrent.remoteOverride);
  arduinoCurrent.occupiedSpots = extractInt(clean, "occupiedSpots", arduinoCurrent.occupiedSpots);
  arduinoCurrent.parkingCapacity = extractInt(clean, "parkingCapacity", arduinoCurrent.parkingCapacity);
  arduinoCurrent.parkingBarrierOpen = extractBool(clean, "parkingBarrierOpen", arduinoCurrent.parkingBarrierOpen);
  arduinoCurrent.vehicleDetected = extractBool(clean, "vehicleDetected", arduinoCurrent.vehicleDetected);
  arduinoCurrent.twilightDetected = extractBool(clean, "twilightDetected", arduinoCurrent.twilightDetected);
  arduinoCurrent.exteriorLightsOn = extractBool(clean, "exteriorLightsOn", arduinoCurrent.exteriorLightsOn);
  arduinoCurrent.awningOpen = extractBool(clean, "awningOpen", arduinoCurrent.awningOpen);
  arduinoCurrent.motionDetected = extractBool(clean, "motionDetected", arduinoCurrent.motionDetected);
  arduinoCurrent.indoorLightsOn = extractBool(clean, "indoorLightsOn", arduinoCurrent.indoorLightsOn);

  hasArduinoData = true;
  serialBaudLocked = true;
  serialStateCount++;
  String msg = "Ricevuto STATE da Arduino baud=" + String(BAUD_RATES[currentBaudIndex]);
  msg += " T=" + String(arduinoCurrent.temperature, 1);
  msg += " H=" + String(arduinoCurrent.humidity);
  msg += " fan=" + String(arduinoCurrent.fanOn ? 1 : 0);
  msg += " win=" + String(arduinoCurrent.windowsOpen ? 1 : 0);
  msg += " door=" + String(arduinoCurrent.internalDoorUnlocked ? 1 : 0);
  msg += " rfid=" + String(arduinoCurrent.rfidReaderEnabled ? 1 : 0);
  msg += " park=" + String(arduinoCurrent.occupiedSpots) + "/" + String(arduinoCurrent.parkingCapacity);
  msg += " barrier=" + String(arduinoCurrent.parkingBarrierOpen ? 1 : 0);
  msg += " vehicle=" + String(arduinoCurrent.vehicleDetected ? 1 : 0);
  msg += " twilight=" + String(arduinoCurrent.twilightDetected ? 1 : 0);
  msg += " lights=" + String(arduinoCurrent.exteriorLightsOn ? 1 : 0);
  msg += " awning=" + String(arduinoCurrent.awningOpen ? 1 : 0);
  msg += " motion=" + String(arduinoCurrent.motionDetected ? 1 : 0);
  msg += " indoor=" + String(arduinoCurrent.indoorLightsOn ? 1 : 0);
  if (DEBUG_VERBOSE || serialStateCount == 1) {
    log("SERIAL", msg);
  }
}

void beginArduinoSerial() {
  Serial2.begin(BAUD_RATES[currentBaudIndex], SERIAL_8N1, ESP_RX_PIN, ESP_TX_PIN);
  lastBaudScan = millis();
  serialBuffer = "";
  serialByteSeen = false;
  log("SERIAL", "Ascolto Arduino a baud " + String(BAUD_RATES[currentBaudIndex]));
}

void scanBaudRateIfNeeded() {
  if (serialBaudLocked) return;
  if (millis() - lastBaudScan < BAUD_SCAN_MS) return;

  currentBaudIndex = (currentBaudIndex + 1) % BAUD_RATE_COUNT;
  Serial2.end();
  beginArduinoSerial();
}

void readSerial() {
  while (Serial2.available()) {
    char c = Serial2.read();
    serialByteSeen = true;

    if (DEBUG_VERBOSE && !hasArduinoData) {
      Serial.print("[ARDUINO RAW] 0x");
      if ((uint8_t)c < 16) Serial.print('0');
      Serial.print((uint8_t)c, HEX);
      Serial.print(" '");
      if (c >= 32 && c <= 126) {
        Serial.print(c);
      } else {
        Serial.print('.');
      }
      Serial.println("'");
    }

    if (c == '\n') {
      parseArduinoMessage(serialBuffer);
      serialBuffer = "";
    } else if (c != '\r') {
      serialBuffer += c;
      if (serialBuffer.length() > 220) {
        serialBuffer = "";
      }
    }
  }
}

// ============================================================
// INVIO COMANDI AD ARDUINO
// ============================================================
void transmitArduinoCommand(String cmd) {
  cmd += "\n";
  Serial2.print(cmd);
  sentCommandCount++;
  log("SERIAL", "Inviato CMD ad Arduino: " + cmd);
}

void sendCommandToArduino(bool buzzerRequest) {
  String cmd = "CMD;";
  bool hasCommand = false;

  if (remoteHas.fanOn &&
      (!commandSnapshotInitialized || remoteDesired.fanOn != lastCommandSnapshot.fanOn)) {
    cmd += "fanOn=" + String(remoteDesired.fanOn ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.windowsOpen &&
      (!commandSnapshotInitialized || remoteDesired.windowsOpen != lastCommandSnapshot.windowsOpen)) {
    cmd += "windowsOpen=" + String(remoteDesired.windowsOpen ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.lcdEnabled &&
      (!commandSnapshotInitialized || remoteDesired.lcdEnabled != lastCommandSnapshot.lcdEnabled)) {
    cmd += "lcdEnabled=" + String(remoteDesired.lcdEnabled ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.internalDoorUnlocked &&
      (!commandSnapshotInitialized || remoteDesired.internalDoorUnlocked != lastCommandSnapshot.internalDoorUnlocked)) {
    cmd += "internalDoorUnlocked=" + String(remoteDesired.internalDoorUnlocked ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.intrusionAlarmArmed &&
      (!commandSnapshotInitialized || remoteDesired.intrusionAlarmArmed != lastCommandSnapshot.intrusionAlarmArmed)) {
    cmd += "intrusionAlarmArmed=" + String(remoteDesired.intrusionAlarmArmed ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.rfidReaderEnabled &&
      (!commandSnapshotInitialized || remoteDesired.rfidReaderEnabled != lastCommandSnapshot.rfidReaderEnabled)) {
    cmd += "rfidReaderEnabled=" + String(remoteDesired.rfidReaderEnabled ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.parkingBarrierOpen &&
      (!commandSnapshotInitialized || remoteDesired.parkingBarrierOpen != lastCommandSnapshot.parkingBarrierOpen)) {
    cmd += "parkingBarrierOpen=" + String(remoteDesired.parkingBarrierOpen ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.parkingCapacity &&
      (!commandSnapshotInitialized || remoteDesired.parkingCapacity != lastCommandSnapshot.parkingCapacity)) {
    cmd += "parkingCapacity=" + String(remoteDesired.parkingCapacity) + ";";
    hasCommand = true;
  }
  if (remoteHas.occupiedSpots &&
      (!commandSnapshotInitialized || remoteDesired.occupiedSpots != lastCommandSnapshot.occupiedSpots)) {
    cmd += "occupiedSpots=" + String(remoteDesired.occupiedSpots) + ";";
    hasCommand = true;
  }
  if (remoteHas.exteriorLightsOn &&
      (!commandSnapshotInitialized || remoteDesired.exteriorLightsOn != lastCommandSnapshot.exteriorLightsOn)) {
    cmd += "exteriorLightsOn=" + String(remoteDesired.exteriorLightsOn ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.awningOpen &&
      (!commandSnapshotInitialized || remoteDesired.awningOpen != lastCommandSnapshot.awningOpen)) {
    cmd += "awningOpen=" + String(remoteDesired.awningOpen ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (remoteHas.indoorLightsOn &&
      (!commandSnapshotInitialized || remoteDesired.indoorLightsOn != lastCommandSnapshot.indoorLightsOn)) {
    cmd += "indoorLightsOn=" + String(remoteDesired.indoorLightsOn ? 1 : 0) + ";";
    hasCommand = true;
  }
  if (hasCommand) {
    transmitArduinoCommand(cmd);
    lastCommandSnapshot = remoteDesired;
    commandSnapshotInitialized = true;
  }

  if (buzzerRequest) {
    String buzzerCmd = "CMD;playBuzzer=1;";
    if (remoteHas.buzzerMelody) {
      buzzerCmd += "buzzerMelody=" + remoteDesired.buzzerMelody + ";";
    }
    if (remoteHas.buzzerSpeed) {
      buzzerCmd += "buzzerSpeed=" + String(remoteDesired.buzzerSpeed) + ";";
    }
    if (remoteHas.buzzerEnabled || remoteHas.doomBuzzerEnabled) {
      bool enabled = remoteHas.buzzerEnabled
        ? remoteDesired.buzzerEnabled
        : remoteDesired.doomBuzzerEnabled;
      buzzerCmd += "buzzerEnabled=" + String(enabled ? 1 : 0) + ";";
      buzzerCmd += "doomBuzzerEnabled=" + String(enabled ? 1 : 0) + ";";
    }
    transmitArduinoCommand(buzzerCmd);
  }
}

// ============================================================
// COMUNICAZIONE SERVER (PULL & PUSH)
// ============================================================

void serverPull() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();  // Per Altervista HTTPS

  http.begin(client, String(SERVER_BASE_URL) + "/api/state.php");
  int httpCode = http.GET();
  lastPullHttp = httpCode;

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);
    JsonObject state = doc["state"];
    remoteHas = DesiredFieldFlags();

    // Legge cosa vuole il server
    if (!state["fanOn"].isNull()) {
      remoteHas.fanOn = true;
      remoteDesired.fanOn = state["fanOn"] | false;
    }
    if (!state["windowsOpen"].isNull()) {
      remoteHas.windowsOpen = true;
      remoteDesired.windowsOpen = state["windowsOpen"] | false;
    }
    if (!state["lcdEnabled"].isNull()) {
      remoteHas.lcdEnabled = true;
      remoteDesired.lcdEnabled = state["lcdEnabled"] | true;
    }
    if (!state["internalDoorUnlocked"].isNull()) {
      remoteHas.internalDoorUnlocked = true;
      remoteDesired.internalDoorUnlocked = state["internalDoorUnlocked"] | false;
    }
    if (!state["intrusionAlarmArmed"].isNull()) {
      remoteHas.intrusionAlarmArmed = true;
      remoteDesired.intrusionAlarmArmed = state["intrusionAlarmArmed"] | true;
    }
    if (!state["rfidReaderEnabled"].isNull()) {
      remoteHas.rfidReaderEnabled = true;
      remoteDesired.rfidReaderEnabled = state["rfidReaderEnabled"] | true;
    }
    if (!state["buzzerRequestId"].isNull()) {
      remoteDesired.buzzerRequestId = state["buzzerRequestId"] | 0;
    }
    if (!state["buzzerMelody"].isNull()) {
      remoteHas.buzzerMelody = true;
      remoteDesired.buzzerMelody = state["buzzerMelody"] | "musicBox";
    }
    if (!state["buzzerSpeed"].isNull()) {
      remoteHas.buzzerSpeed = true;
      remoteDesired.buzzerSpeed = constrain(state["buzzerSpeed"] | 100, 50, 200);
    }
    if (!state["buzzerEnabled"].isNull()) {
      remoteHas.buzzerEnabled = true;
      remoteDesired.buzzerEnabled = state["buzzerEnabled"] | false;
    }
    if (!state["doomBuzzerEnabled"].isNull()) {
      remoteHas.doomBuzzerEnabled = true;
      remoteDesired.doomBuzzerEnabled = state["doomBuzzerEnabled"] | false;
    }
    if (!state["parkingBarrierOpen"].isNull()) {
      remoteHas.parkingBarrierOpen = true;
      remoteDesired.parkingBarrierOpen = state["parkingBarrierOpen"] | false;
    }
    if (!state["parkingCapacity"].isNull()) {
      remoteHas.parkingCapacity = true;
      remoteDesired.parkingCapacity = state["parkingCapacity"] | remoteDesired.parkingCapacity;
    }
    if (!state["occupiedSpots"].isNull()) {
      remoteHas.occupiedSpots = true;
      remoteDesired.occupiedSpots = state["occupiedSpots"] | remoteDesired.occupiedSpots;
    }
    if (!state["exteriorLightsOn"].isNull()) {
      remoteHas.exteriorLightsOn = true;
      remoteDesired.exteriorLightsOn = state["exteriorLightsOn"] | false;
    }
    if (!state["awningOpen"].isNull()) {
      remoteHas.awningOpen = true;
      remoteDesired.awningOpen = state["awningOpen"] | true;
    }
    if (!state["indoorLightsOn"].isNull()) {
      remoteHas.indoorLightsOn = true;
      remoteDesired.indoorLightsOn = state["indoorLightsOn"] | false;
    }

    bool buzzerRequest =
      hasArduinoData && remoteDesired.buzzerRequestId > lastForwardedBuzzerRequestId;
    sendCommandToArduino(buzzerRequest);
    if (buzzerRequest) {
      lastForwardedBuzzerRequestId = remoteDesired.buzzerRequestId;
    }
    pullOkCount++;
  } else {
    pullFailCount++;
    log("HTTP", "Errore Pull: " + String(httpCode));
  }
  http.end();
}

void serverPush() {
  if (WiFi.status() != WL_CONNECTED) {
    log("HTTP", "PUSH saltato: WiFi non connesso");
    return;
  }

  if (!hasArduinoData) {
    if (serialByteSeen) {
      log(
        "HTTP",
        "PUSH saltato: byte da Arduino ricevuti, ma nessuna riga STATE valida. Buffer=" +
          String(serialBuffer.length()) + " '" + printablePreview(serialBuffer) + "'"
      );
    } else {
      log("HTTP", "PUSH saltato: nessun byte ricevuto da Arduino");
    }
    return;
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  http.begin(client, String(SERVER_BASE_URL) + "/api/device_state.php");
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(2048);
  doc["temperature"] = arduinoCurrent.temperature;
  doc["humidity"] = arduinoCurrent.humidity;
  doc["sensorOk"] = arduinoCurrent.sensorOk;
  doc["fanOn"] = arduinoCurrent.fanOn;
  doc["fanPower"] = arduinoCurrent.fanPower;
  doc["fanPwm"] = arduinoCurrent.fanPwm;
  doc["windowsOpen"] = arduinoCurrent.windowsOpen;
  doc["internalDoorUnlocked"] = arduinoCurrent.internalDoorUnlocked;
  doc["intrusionAlarmArmed"] = arduinoCurrent.intrusionAlarmArmed;
  doc["rfidReaderEnabled"] = arduinoCurrent.rfidReaderEnabled;
  doc["lcdEnabled"] = arduinoCurrent.lcdEnabled;
  doc["remoteOverride"] = arduinoCurrent.remoteOverride;
  doc["occupiedSpots"] = arduinoCurrent.occupiedSpots;
  doc["parkingCapacity"] = arduinoCurrent.parkingCapacity;
  doc["parkingBarrierOpen"] = arduinoCurrent.parkingBarrierOpen;
  doc["vehicleDetected"] = arduinoCurrent.vehicleDetected;
  doc["twilightDetected"] = arduinoCurrent.twilightDetected;
  doc["exteriorLightsOn"] = arduinoCurrent.exteriorLightsOn;
  doc["awningOpen"] = arduinoCurrent.awningOpen;
  doc["motionDetected"] = arduinoCurrent.motionDetected;
  doc["indoorLightsOn"] = arduinoCurrent.indoorLightsOn;

  String json;
  serializeJson(doc, json);

  int httpCode = http.POST(json);
  lastPushHttp = httpCode;
  if (httpCode != 200) {
    pushFailCount++;
    log("HTTP", "Errore Push: " + String(httpCode));
  } else {
    pushOkCount++;
    if (DEBUG_VERBOSE) {
      log("HTTP", "Push OK: " + json);
    }
  }
  http.end();
}

// ============================================================
// SETUP E LOOP
// ============================================================

void setup() {
  Serial.begin(115200);  // Monitor Seriale PC
  beginArduinoSerial();

  log("BOOT", "ESP32 avviato. Connessione WiFi...");
  log("BOOT", "Serial2 ESP32: RX GPIO16 <- TX Arduino, TX GPIO17 -> RX Arduino");
  log("BOOT", "Mega Serial1: TX1 pin 18 -> ESP32 RX16, RX1 pin 19 <- ESP32 TX17");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  log("WIFI", "Connesso! IP: " + WiFi.localIP().toString());
  logStatusSummary();
}

void loop() {
  readSerial();  // Ascolta sempre Arduino
  scanBaudRateIfNeeded();

  unsigned long now = millis();

  // Gestione Pull dal Server
  if (now - lastPull >= INTERVALLO_PULL) {
    serverPull();
    lastPull = now;
  }

  // Gestione Push al Server
  if (now - lastPush >= INTERVALLO_PUSH) {
    serverPush();
    lastPush = now;
  }

  if (now - lastStatusLog >= STATUS_LOG_MS) {
    logStatusSummary();
    lastStatusLog = now;
  }
}
