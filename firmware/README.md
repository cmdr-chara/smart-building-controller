# Firmware

Questa cartella contiene gli sketch hardware del progetto. Ogni sottocartella
e' un modulo indipendente: apri il file `.ino` della cartella giusta e caricalo
sulla scheda indicata.

## Moduli disponibili

| Modulo | Cartella | File | Scheda | Collegamento server |
| --- | --- | --- | --- | --- |
| Porta interna, RFID, allarme | `accessi_allarme/` | `accessi_allarme.ino` | ESP32 | Wi-Fi diretto |
| Parcheggio, RFID, sbarra, posti | `parcheggio_sbarra/` | `parcheggio_sbarra.ino` | Arduino Mega | bridge ESP32 |
| Luci esterne e tenda | `esterni_tenda/` | `esterni_tenda.ino` | Arduino Uno/compatibile | bridge ESP32 |
| Clima, OLED, ventola, finestre, buzzer | `clima_ventola_finestre/` | `clima_ventola_finestre.ino` | Arduino Mega | bridge ESP32 |
| Bridge app/server | `bridge_esp32_server/` | `bridge_esp32_server.ino` | ESP32 | Wi-Fi + seriale |
| Luci interne | `luci_interne/` | non ancora presente | da decidere | bridge ESP32 previsto |

## Architettura firmware

Ci sono due modi di collegarsi al server:

```text
Modulo ESP32 autonomo:
accessi_allarme -> Wi-Fi -> server PHP

Modulo Arduino tramite bridge:
Arduino Uno/Mega -> seriale STATE/CMD -> ESP32 bridge -> Wi-Fi -> server PHP
```

Il bridge riceve dal server lo stato desiderato e lo manda al modulo Arduino con
righe `CMD;...`. Il modulo Arduino pubblica il proprio stato con righe
`STATE;...`.

## Procedura base Arduino IDE

1. Apri Arduino IDE.
2. Vai su `File > Apri`.
3. Seleziona il file `.ino` del modulo.
4. Seleziona la scheda corretta da `Strumenti > Scheda`.
5. Seleziona la porta USB.
6. Installa le librerie richieste dal README del modulo.
7. Clicca `Verifica`.
8. Clicca `Carica`.
9. Apri il Monitor Seriale al baud indicato.

## Baud seriali

| Collegamento | Baud |
| --- | --- |
| Monitor Seriale ESP32 bridge | `115200` |
| Monitor Seriale accessi/allarme | `115200` |
| Monitor Seriale moduli Arduino | `115200` |
| Bridge ESP32 <-> clima Mega | `57600` |
| Bridge ESP32 <-> parcheggio Mega | `9600` |
| Bridge ESP32 <-> esterni Uno | `9600` |

Il bridge scansiona automaticamente `9600` e `57600` finche' trova una riga
`STATE;...` valida.

## Pin: accessi, RFID e allarme

File: `accessi_allarme/accessi_allarme.ino`

Scheda: ESP32

| Componente | Pin ESP32 |
| --- | --- |
| RFID SS/SDA | GPIO5 |
| RFID RST | GPIO22 |
| RFID SCK | GPIO18 |
| RFID MISO | GPIO19 |
| RFID MOSI | GPIO23 |
| LED verde | GPIO32 |
| LED rosso | GPIO33 |
| Buzzer | GPIO26 |
| Stepper porta IN1 | GPIO13 |
| Stepper porta IN2 | GPIO14 |
| Stepper porta IN3 | GPIO12 |
| Stepper porta IN4 | GPIO27 |
| LCD RS | GPIO21 |
| LCD E | GPIO17 |
| LCD D4 | GPIO16 |
| LCD D5 | GPIO4 |
| LCD D6 | GPIO2 |
| LCD D7 | GPIO15 |

Questo modulo usa Wi-Fi direttamente e non richiede il bridge seriale. Lo
stepper 28BYJ-48 con ULN2003 e' cablato come `IN1, IN2, IN3, IN4`, ma la
libreria `Stepper` viene inizializzata come `IN1, IN3, IN2, IN4`.

## Pin: parcheggio, RFID, sbarra e posti

File: `parcheggio_sbarra/parcheggio_sbarra.ino`

Scheda: Arduino Mega

| Componente | Pin scheda |
| --- | --- |
| RFID SS/SDA | 53 |
| RFID RST | 30 |
| LCD RS | 31 |
| LCD E | 33 |
| LCD D4 | 35 |
| LCD D5 | 37 |
| LCD D6 | 39 |
| LCD D7 | 41 |
| Stepper sbarra IN1 | 11 |
| Stepper sbarra IN2 | 9 |
| Stepper sbarra IN3 | 10 |
| Stepper sbarra IN4 | 8 |
| Ultrasuoni ingresso TRIG | 2 |
| Ultrasuoni ingresso ECHO | 3 |
| LED ingresso | 5 |
| Ultrasuoni uscita TRIG | 13 |
| Ultrasuoni uscita ECHO | 12 |
| LED uscita | 4 |
| ESP32 TX2 GPIO17 | Mega RX1 pin 19 |
| Mega TX1 pin 18 | partitore 1k/2k -> ESP32 RX2 GPIO16 |
| GND ESP32 | GND Mega |

## Pin: luci esterne e tenda

File: `esterni_tenda/esterni_tenda.ino`

Scheda: Arduino Uno o compatibile con seriale hardware sui pin 0/1

| Componente | Pin scheda |
| --- | --- |
| Sensore crepuscolare | A0 |
| LED esterno 1 | 2 |
| LED esterno 2 | 3 |
| LED esterno 3 | 4 |
| LED esterno 4 | 5 |
| Stepper tenda IN1 | 8 |
| Stepper tenda IN2 | 10 |
| Stepper tenda IN3 | 9 |
| Stepper tenda IN4 | 11 |
| ESP32 TX2 GPIO17 | Arduino RX pin 0 |
| Arduino TX pin 1 | partitore 1k/2k -> ESP32 RX2 GPIO16 |
| GND ESP32 | GND Arduino |

Scollega i fili dai pin 0/1 durante l'upload sull'Arduino.

## Pin: temperatura, umidita', ventola e finestre

File: `clima_ventola_finestre/clima_ventola_finestre.ino`

Scheda: Arduino Mega

| Componente | Pin scheda |
| --- | --- |
| OLED SSD1306 SDA | SDA della scheda |
| OLED SSD1306 SCL | SCL della scheda |
| DHT11 DATA | 8 |
| Ventola / driver ventola | 9 |
| Buzzer | 10 |
| Stepper finestre IN1 | A0 |
| Stepper finestre IN2 | A2 |
| Stepper finestre IN3 | A1 |
| Stepper finestre IN4 | A3 |
| ESP32 TX2 GPIO17 | Mega RX1 pin 19 |
| Mega TX1 pin 18 | partitore 1k/2k -> ESP32 RX2 GPIO16 |
| GND ESP32 | GND Mega |

Pin I2C tipici:

| Scheda | SDA | SCL |
| --- | --- | --- |
| Arduino Uno/Nano | A4 | A5 |
| Arduino Mega | 20 | 21 |

## Pin: bridge ESP32 server

File: `bridge_esp32_server/bridge_esp32_server.ino`

Scheda: ESP32

| Funzione | Pin ESP32 |
| --- | --- |
| RX2 dal modulo Arduino | GPIO16 |
| TX2 verso modulo Arduino | GPIO17 |
| Massa comune | GND |

I pin lato Arduino cambiano in base al modulo collegato. Usa la tabella del
modulo corrispondente.

## Protocollo seriale

Stato Arduino verso bridge:

```text
STATE;temperature=24;humidity=50;fanOn=1;windowsOpen=0;
STATE;parkingBarrierOpen=0;occupiedSpots=3;parkingCapacity=20;
STATE;twilightDetected=1;exteriorLightsOn=1;awningOpen=0;
```

Comando bridge verso Arduino:

```text
CMD;fanOn=1;windowsOpen=0;
CMD;parkingBarrierOpen=1;rfidReaderEnabled=1;
CMD;exteriorLightsOn=1;awningOpen=0;
CMD;playBuzzer=1;buzzerMelody=musicBox;buzzerSpeed=100;buzzerEnabled=1;
```

Ogni riga termina con newline.

## Partitore 5V -> 3.3V

Quando un TX Arduino/Mega entra nell'RX dell'ESP32, usa:

```text
TX Arduino/Mega -> resistenza 1k -> nodo centrale -> ESP32 RX2 GPIO16
nodo centrale -> resistenza 2k -> GND
GND Arduino/Mega -> GND ESP32
```

Serve per proteggere l'ESP32, che non e' tollerante a 5V sui GPIO.

## URL server ESP32

Nel bridge e nel modulo accessi imposta:

```cpp
const char* SERVER_BASE_URL = "https://TUO_DOMINIO.altervista.org/smart-controller";
```

Non aggiungere `/api`: gli sketch aggiungono da soli `/api/state.php` e
`/api/device_state.php`.

## Compilazione con arduino-cli

Esempi:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\bridge_esp32_server
arduino-cli compile --fqbn esp32:esp32:esp32 firmware\accessi_allarme
arduino-cli compile --fqbn arduino:avr:mega firmware\parcheggio_sbarra
arduino-cli compile --fqbn arduino:avr:uno firmware\esterni_tenda
arduino-cli compile --fqbn arduino:avr:mega firmware\clima_ventola_finestre
```

Se `arduino-cli` non trova una libreria, installala dal Library Manager o con:

```powershell
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "MFRC522"
arduino-cli lib install "DHT11"
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SSD1306"
```

## Debug rapido

- Se il bridge dice `arduino=waiting`, controlla fili TX/RX/GND.
- Se il bridge dice `noise`, riceve byte ma non righe `STATE;...` valide.
- Se `lastHttp` non e' `200/200`, controlla URL server e PHP.
- Se l'app non cambia, apri `state.php` e controlla `lastUpdated`.
- Se un comando viene sovrascritto subito, controlla `_commandLocks` nel JSON.
