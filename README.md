# Smart Building Controller

Flutter app, PHP backend and Arduino/ESP32 firmware for a school smart-building
prototype. The system controls and monitors access, alarm, parking, lights,
awning, climate, windows, fan and buzzer modules.

```text
Flutter app <-> PHP API <-> ESP32 bridge <-> Arduino modules <-> Sensors/actuators
```

The `accessi_allarme` module is the exception: it runs directly on ESP32 and
talks to the PHP API over Wi-Fi without the serial bridge.

## Project Layout

```text
lib/                         Flutter application
server/                      PHP API and JSON storage
firmware/                    ESP32 and Arduino sketches
docs/schema_elettrico.md     Mermaid wiring diagrams and pin tables
assets/branding/             App icon source assets
test/                        Flutter tests
```

Main firmware modules:

```text
firmware/bridge_esp32_server/        ESP32 bridge for API <-> serial modules
firmware/accessi_allarme/            Access, RFID and alarm module on ESP32
firmware/parcheggio_sbarra/          Parking barrier module
firmware/esterni_tenda/              Exterior lights and awning module
firmware/clima_ventola_finestre/     Climate, fan, windows, OLED and buzzer
firmware/luci_interne/               Placeholder for a future module
```

## How It Works

1. The app reads state from authenticated `GET /api/state.php`.
2. The app sends commands to authenticated `POST /api/command.php`.
3. The PHP API persists the desired state in `server/storage/state.json`.
4. The ESP32 bridge polls the API and sends `CMD;...` lines to Arduino.
5. Arduino modules reply with `STATE;...` lines.
6. The bridge posts live device state to authenticated `POST /api/device_state.php`.
7. The app refreshes and displays the updated state.

Example serial command:

```text
CMD;fanOn=1;windowsOpen=0;buzzerMelody=musicBox;buzzerSpeed=100;playBuzzer=1;
```

Example serial state:

```text
STATE;temperature=26;humidity=52;sensorOk=1;fanOn=1;windowsOpen=0;module=clima;board=mega;
```

## Backend Setup

Upload the contents of `server/` to a web folder named `smart-controller`.

Expected hosted layout:

```text
smart-controller/
  api/
    config.php
    state.php
    command.php
    device_state.php
  storage/
    state.json
```

Base URL examples:

```text
https://TUO_DOMINIO.altervista.org/smart-controller
http://127.0.0.1/smart-controller
```

Use the base URL in the app settings. Do not append `/api`, `state.php` or
`command.php`; the app builds those endpoint URLs internally. Remote endpoints
must use HTTPS; cleartext HTTP is accepted by the Flutter client only for
loopback development.

### API security configuration

The PHP API fails closed unless `SMART_CONTROLLER_API_TOKEN` is configured with
at least 32 characters. Use the same value for the Flutter build and both ESP32
clients. Do not commit it.

Example server environment:

```text
SMART_CONTROLLER_API_TOKEN=replace-with-a-random-32-plus-character-token
SMART_CONTROLLER_ALLOWED_ORIGIN=https://optional-browser-origin.example
```

`SMART_CONTROLLER_ALLOWED_ORIGIN` is optional. If it is unset, the API does not
emit a cross-origin browser allowlist. Native Flutter and ESP32 clients do not
need CORS.

The API also limits JSON request bodies, uses restrictive storage permissions,
and requires the bearer token for reads, commands and device-state writes.

## Flutter App

Install dependencies:

```powershell
flutter pub get
```

Run the app with the same API token configured on the server:

```powershell
flutter run --dart-define=SMART_CONTROLLER_API_TOKEN=replace-with-the-same-token
```

Build a release APK:

```powershell
flutter build apk --release --dart-define=SMART_CONTROLLER_API_TOKEN=replace-with-the-same-token
```

The token is compiled into the client and therefore should be treated as a
prototype access control, not as a substitute for per-user authentication in a
real building deployment.

Useful checks:

```powershell
flutter analyze
flutter test
```

Main packages:

```text
flutter_riverpod
http
shared_preferences
google_fonts
flutter_animate
liquid_glass_widgets
```

## Firmware Setup

Configure Wi-Fi and the HTTPS server URL in each networked ESP32 sketch before
uploading:

```cpp
const char* WIFI_SSID = "NOME_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";
const char* SERVER_BASE_URL = "https://TUO_DOMINIO.altervista.org/smart-controller";
```

For both `firmware/bridge_esp32_server/` and `firmware/accessi_allarme/`, copy
`secrets.h.example` to `secrets.h`, then configure:

```cpp
const char* API_TOKEN = "replace-with-the-same-32-plus-character-token";
const char* TLS_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
...root CA that validates SERVER_BASE_URL...
-----END CERTIFICATE-----
)EOF";
```

`secrets.h` is ignored by Git. The ESP32 clients fail closed when the token, HTTPS
URL or CA certificate is missing. TLS certificate verification is enabled; do
not replace `setCACert()` with `setInsecure()`.

Bridge sketch:

```text
firmware/bridge_esp32_server/bridge_esp32_server.ino
Board: ESP32 Dev Module
Library: ArduinoJson
Serial monitor: 115200 baud
```

The bridge uses ESP32 `Serial2`:

```cpp
const uint8_t ESP_RX_PIN = 16;
const uint8_t ESP_TX_PIN = 17;
```

Arduino module sketches live in their own folders under `firmware/`. Each folder
has a README with module-specific wiring, libraries and diagnostics.

## Wiring Notes

For Arduino Mega modules using `Serial1`:

```text
Mega TX1 pin 18  -> voltage divider -> ESP32 GPIO16 RX2
Mega RX1 pin 19  <- direct          <- ESP32 GPIO17 TX2
GND Mega         -> GND ESP32
```

For Arduino Uno modules using pins `0/1`:

```text
Uno TX pin 1 -> voltage divider -> ESP32 GPIO16 RX2
Uno RX pin 0 <- direct          <- ESP32 GPIO17 TX2
GND Uno      -> GND ESP32
```

Arduino TX is 5V while ESP32 RX is 3.3V, so use a voltage divider on the
Arduino-to-ESP32 line. Disconnect Uno pins `0/1` while uploading sketches.

## Documentation

- [Electrical schema](docs/schema_elettrico.md)
- [Bridge firmware](firmware/bridge_esp32_server/README.md)
- [Access and alarm module](firmware/accessi_allarme/README.md)
- [Climate module](firmware/clima_ventola_finestre/README.md)
- [Exterior lights and awning module](firmware/esterni_tenda/README.md)
- [Parking module](firmware/parcheggio_sbarra/README.md)

## Troubleshooting

If the app or ESP32 cannot reach the API:

1. Confirm `SMART_CONTROLLER_API_TOKEN` is set on the server and the same token is used by the client.
2. Confirm the remote URL uses HTTPS.
3. Confirm each ESP32 `secrets.h` contains the correct root CA for the server certificate.
4. Check ESP32 serial logs for `security=ok`, pull/push counters and HTTP status values.
5. Check that `lastUpdated` changes after authenticated device writes.

If ESP32 receives no valid Arduino state:

- check crossed TX/RX wiring;
- check common GND;
- check the voltage divider on Arduino TX;
- check the baud rate used by the module;
- confirm the sketch is sending `STATE;...` lines.

If ESP32 upload fails:

1. Close Serial Monitor.
2. Disconnect GPIO16/GPIO17 temporarily.
3. Retry upload.
4. If it stays on `Connecting...`, hold `BOOT` until upload starts.

## Maintenance Commands

PHP syntax checks, with a local PHP binary:

```powershell
php -l server\api\config.php
php -l server\api\state.php
php -l server\api\command.php
php -l server\api\device_state.php
```

Arduino CLI examples (after creating each required `secrets.h`):

```powershell
arduino-cli compile --fqbn arduino:avr:mega firmware\clima_ventola_finestre
arduino-cli compile --fqbn arduino:avr:mega firmware\parcheggio_sbarra
arduino-cli compile --fqbn arduino:avr:uno firmware\esterni_tenda
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\bridge_esp32_server
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\accessi_allarme
```
