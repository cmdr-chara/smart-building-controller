# Smart Controller App

App Flutter per controllare un impianto scolastico con moduli Arduino/ESP32,
sensori, attuatori e backend PHP. Il progetto e' pensato per funzionare sia in
locale con Laragon/XAMPP sia online su Altervista.

La catena principale e':

```text
App Flutter <-> Server PHP <-> ESP32 bridge <-> Arduino Mega/Uno <-> Sensori/attuatori
```

Il modulo `accessi_allarme` e' diverso: gira direttamente su ESP32, si collega
al server via Wi-Fi e non usa il bridge seriale separato.

## Cosa contiene il progetto

```text
lib/                                   App Flutter
assets/branding/app_icon.png           Icona/app asset
docs/schema_elettrico.md               Schemi Mermaid e tabelle collegamenti
server/                                Backend PHP da caricare su hosting
server/api/state.php                   Lettura stato per app ed ESP32
server/api/command.php                 Comandi inviati dall'app
server/api/device_state.php            Stato reale inviato dai dispositivi
server/storage/state.json              Stato persistente
firmware/bridge_esp32_server/          Bridge ESP32 server <-> Arduino
firmware/accessi_allarme/              Porta interna, RFID e allarme su ESP32
firmware/parcheggio_sbarra/            Parcheggio con RFID, LCD, ultrasuoni
firmware/esterni_tenda/                Luci esterne e tenda su Arduino Uno
firmware/clima_ventola_finestre/       Clima, OLED, ventola, finestre, buzzer
firmware/luci_interne/                 Placeholder per modulo futuro
```

## Schema elettrico

Gli schemi elettrici sono in:

```text
docs/schema_elettrico.md
```

Il documento usa diagrammi Mermaid divisi per modulo e tabelle pin sotto ogni
schema. In questo modo resta leggibile su GitHub senza dover usare Fritzing o
KiCad.

## Flusso dati

1. L'app legge `GET /api/state.php`.
2. L'app invia comandi con `POST /api/command.php`.
3. `command.php` aggiorna `storage/state.json` con il comando desiderato.
4. L'ESP32 bridge legge periodicamente `state.php`.
5. Il bridge trasforma lo stato desiderato in righe seriali `CMD;...`.
6. Il modulo Arduino risponde con righe `STATE;...`.
7. Il bridge pubblica lo stato reale con `POST /api/device_state.php`.
8. L'app rilegge `state.php` e mostra lo stato aggiornato.

Esempio di comando seriale:

```text
CMD;fanOn=1;windowsOpen=0;buzzerMelody=musicBox;buzzerSpeed=100;playBuzzer=1;
```

Esempio di stato seriale:

```text
STATE;temperature=26;humidity=52;sensorOk=1;fanOn=1;windowsOpen=0;module=clima;board=mega;
```

## Preparare il server PHP

Carica la cartella `server` sul tuo hosting dentro una cartella chiamata:

```text
smart-controller
```

Su Altervista la struttura finale deve essere:

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

Non mettere `public_html` nell'URL. L'endpoint corretto deve essere:

```text
https://TUO_DOMINIO.altervista.org/smart-controller/api/state.php
```

Se funziona, il browser mostra JSON simile a:

```json
{
  "ok": true,
  "state": {
    "temperature": 27.2,
    "humidity": 62,
    "fanOn": true,
    "lastUpdated": "2026-05-28T12:00:00+00:00"
  }
}
```

Per uso locale con Laragon/XAMPP, copia `server` come:

```text
C:\laragon\www\smart-controller
```

oppure:

```text
C:\xampp\htdocs\smart-controller
```

Poi usa:

```text
http://127.0.0.1/smart-controller
```

## Configurare e avviare l'app Flutter

Installa dipendenze:

```powershell
flutter pub get
```

Avvia l'app:

```powershell
flutter run
```

Nelle impostazioni dell'app inserisci solo l'endpoint base:

```text
https://TUO_DOMINIO.altervista.org/smart-controller
```

Non aggiungere `/api`, `/api/state.php` o `/api/command.php`: l'app normalizza
l'URL e aggiunge gli endpoint corretti da sola.

Per generare un APK:

```powershell
flutter build apk --release
```

Dipendenze principali:

```text
flutter_riverpod      stato app e controller
http                  chiamate API PHP
shared_preferences    salvataggio base URL
google_fonts          tipografia
flutter_animate       animazioni UI
liquid_glass_widgets  navigazione/stile glass
```

## Caricare il bridge ESP32

Apri:

```text
firmware/bridge_esp32_server/bridge_esp32_server.ino
```

Scheda:

```text
ESP32 Dev Module
```

Installa la libreria:

```text
ArduinoJson
```

Nel file imposta Wi-Fi e server:

```cpp
const char* WIFI_SSID = "NOME_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";
const char* SERVER_BASE_URL = "https://TUO_DOMINIO.altervista.org/smart-controller";
```

Il bridge usa `Serial2` su ESP32:

```cpp
const uint8_t ESP_RX_PIN = 16;
const uint8_t ESP_TX_PIN = 17;
const unsigned long BAUD_RATES[] = {9600UL, 57600UL};
```

Il Monitor Seriale dell'ESP32 deve stare a:

```text
115200 baud
```

## Collegare ESP32 e Arduino

Per moduli su Arduino Mega con `Serial1`:

```text
Mega TX1 pin 18  -> partitore -> ESP32 GPIO16 RX2
Mega RX1 pin 19  <- diretto    <- ESP32 GPIO17 TX2
GND Mega         -> GND ESP32
```

Per `esterni_tenda` su Arduino Uno:

```text
Arduino TX pin 1 -> partitore -> ESP32 GPIO16 RX2
Arduino RX pin 0 <- diretto    <- ESP32 GPIO17 TX2
GND Arduino      -> GND ESP32
```

Sui pin `0/1` dell'Arduino Uno passa anche la seriale USB: scollega quei fili
quando carichi lo sketch, poi ricollegali dopo l'upload.

## Partitore 5V -> 3.3V

Il TX di Arduino/Mega lavora a 5V. L'RX dell'ESP32 lavora a 3.3V. Sul filo che
parte dall'Arduino verso ESP32 serve un partitore:

```text
TX Arduino/Mega
   |
 [1k]
   |
   +----> ESP32 GPIO16 RX2
   |
 [2k]
   |
  GND
```

Il segnale ESP32 TX2 -> RX Arduino/Mega puo' andare diretto.

## Modulo clima/ventola/finestre

Apri:

```text
firmware/clima_ventola_finestre/clima_ventola_finestre.ino
```

Scheda:

```text
Arduino Mega or Mega 2560
```

Librerie:

```text
DHT11
Adafruit GFX Library
Adafruit SSD1306
Stepper
```

Lo sketch usa:

```cpp
#define espLink Serial1
espLink.begin(57600);
```

Campi principali inviati:

```text
temperature, humidity, sensorOk, fanOn, fanPower, fanPwm,
windowsOpen, lcdEnabled, remoteOverride, module, board, uptimeMs
```

La ventola usa soglie automatiche `24 C` ON e `23 C` OFF, salvo override
ricevuto dall'ESP32. I comandi manuali restano validi per circa 10 secondi.

## Buzzer nell'app

Nel pannello clima ci sono:

```text
Doom Riff
Grandfather Clock
Toreador March
Megalovania breve
Sirena 118
speed 50% - 200%
Suona / Stop
```

Quando premi `Suona`, l'app invia `playSelectedBuzzer` a `command.php`. Il
server incrementa `buzzerRequestId`, imposta `buzzerMelody`, `buzzerSpeed` e
`buzzerEnabled`, poi il bridge passa il comando al Mega.

Esempio:

```text
App -> command.php -> state.php -> ESP32 bridge -> CMD;playBuzzer=1;buzzerMelody=toreador;buzzerSpeed=125;buzzerEnabled=1; -> Mega
```

Il pulsante `Stop` manda:

```text
CMD;playBuzzer=1;buzzerEnabled=0;doomBuzzerEnabled=0;
```

Il firmware clima controlla i comandi anche durante l'attesa tra le note, quindi
lo stop puo' interrompere una melodia in corso.

## Come capire se funziona

Sul Monitor Seriale dell'ESP32 devi vedere:

```text
[12345 ms] [SERIAL] Ricevuto STATE da Arduino baud=57600 T=26.0 H=36 fan=1 win=1 ...
[30000 ms] [STATUS] wifi=ok arduino=ok baud=57600 locked state=4 ignored=0 cmd=2 pull=4/0 push=4/0 lastHttp=200/200
```

Sul Monitor Seriale del Mega clima:

```text
[CLIMA STATUS] dht=ok temp=26C hum=36% fan=on power=160 pwm=160 windows=closed mode=auto
```

Con `DEBUG_VERBOSE = true`, il bridge stampa anche i push HTTP riusciti e il
Mega stampa le righe `STATE;...` inviate su `Serial1`.

## Problemi comuni

### L'app non aggiorna i dati

1. Apri `https://TUO_DOMINIO.altervista.org/smart-controller/api/state.php`.
2. Controlla che `lastUpdated` cambi.
3. Controlla il riepilogo ESP32: `push=OK/FAIL` e `lastHttp=200/200`.
4. Se `lastUpdated` resta fermo, il problema e' prima dell'app: ESP32, seriale
   Arduino o `device_state.php`.

### ESP32 dice `PUSH saltato: nessun byte ricevuto da Arduino`

Controlla:

- TX/RX incrociati.
- GND comune.
- Partitore sul TX Arduino -> RX ESP32.
- Sketch giusto caricato sul modulo Arduino.
- Baud compatibile (`57600` per clima, `9600` per parcheggio/esterni).

### ESP32 riceve byte ma nessuna riga `STATE`

Probabili cause:

- fili invertiti;
- baud sbagliato;
- sketch Arduino non sta mandando `STATE;`;
- stai guardando il modulo sbagliato;
- sui pin 0/1 di Arduino Uno sono rimasti collegati USB e bridge insieme.

### Upload ESP32 fallisce con `The chip stopped responding`

1. Chiudi il Monitor Seriale.
2. Scollega temporaneamente GPIO16/GPIO17.
3. Premi upload.
4. Se resta su `Connecting...`, tieni premuto `BOOT` finche' parte il caricamento.

### Il buzzer suona quando non vuoi

Nel firmware clima puoi disattivarlo:

```cpp
const bool BUZZER_ENABLED = false;
```

Oppure usa il pulsante `Stop` nell'app.

## Comandi utili

```powershell
flutter analyze
flutter test
flutter build apk --release
```

Lint PHP con Laragon:

```powershell
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\config.php
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\state.php
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\command.php
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\device_state.php
```

Compilazione firmware con `arduino-cli`:

```powershell
arduino-cli compile --fqbn arduino:avr:mega firmware\clima_ventola_finestre
arduino-cli compile --fqbn arduino:avr:mega firmware\parcheggio_sbarra
arduino-cli compile --fqbn arduino:avr:uno firmware\esterni_tenda
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\bridge_esp32_server
```

## Cosa caricare dopo modifiche

Se modifichi il server:

```text
server/api/config.php
server/api/state.php
server/api/command.php
server/api/device_state.php
server/storage/state.json se vuoi cambiare lo stato iniziale
```

Se modifichi il bridge:

```text
firmware/bridge_esp32_server/bridge_esp32_server.ino
```

Se modifichi un modulo hardware:

```text
firmware/accessi_allarme/accessi_allarme.ino
firmware/parcheggio_sbarra/parcheggio_sbarra.ino
firmware/esterni_tenda/esterni_tenda.ino
firmware/clima_ventola_finestre/clima_ventola_finestre.ino
```

Se modifichi l'app:

```powershell
flutter pub get
flutter analyze
flutter test
flutter build apk --release
```
