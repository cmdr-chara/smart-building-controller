# Backend PHP per Laragon, XAMPP e Altervista

Questa cartella contiene il backend minimo usato dall'app Flutter e dagli
ESP32. Non usa database: lo stato vive in `storage/state.json`.

## Struttura

```text
server/
  api/
    config.php        helper comuni, CORS, JSON, storage
    state.php         GET stato corrente
    command.php       POST comandi dall'app
    device_state.php  POST stato reale dai dispositivi
  storage/
    state.json        file JSON persistente
```

## Endpoint

| Endpoint | Metodo | Chi lo usa | Cosa fa |
| --- | --- | --- | --- |
| `/api/state.php` | `GET` | app, ESP32 bridge, accessi ESP32 | restituisce lo stato normalizzato |
| `/api/command.php` | `POST` | app Flutter | applica un comando desiderato |
| `/api/device_state.php` | `POST` | ESP32 bridge, accessi ESP32 | aggiorna lo stato reale del dispositivo |

Tutti gli endpoint rispondono JSON e includono header CORS permissivi, cosi'
l'app puo' parlare con il server anche da web/dev build.

## Flusso normale

```text
App Flutter
  POST /api/command.php {"action":"toggleFan"}
Server PHP
  salva fanOn nel JSON e blocca il campo per pochi secondi
ESP32 bridge
  GET /api/state.php
  invia CMD;fanOn=1; ad Arduino
Arduino
  applica il comando e risponde STATE;fanOn=1;...
ESP32 bridge
  POST /api/device_state.php con lo stato reale
App Flutter
  GET /api/state.php e aggiorna UI
```

`command.php` rappresenta lo stato desiderato. `device_state.php` rappresenta lo
stato osservato dal dispositivo.

## Stato iniziale

Se `storage/state.json` non esiste o non e' JSON valido, `config.php` ricrea uno
stato di default con campi come:

```text
internalDoorUnlocked
intrusionAlarmArmed
rfidReaderEnabled
parkingBarrierOpen
vehicleDetected
parkingCapacity
occupiedSpots
twilightDetected
exteriorLightsOn
awningOpen
temperature
humidity
sensorOk
fanOn
fanPower
fanPwm
windowsOpen
remoteOverride
buzzerRequestId
buzzerMelody
buzzerSpeed
buzzerEnabled
doomBuzzerEnabled
motionDetected
indoorLightsOn
lcdEnabled
lastUpdated
```

I boolean vengono normalizzati, `parkingCapacity` viene limitato a `0..999`,
`occupiedSpots` non puo' superare la capienza, `fanPower` e `fanPwm` vengono
limitati a `0..255`, `buzzerSpeed` viene limitato a `50..200` e `lastUpdated`
viene aggiornato a ogni salvataggio.

## Comandi supportati

`command.php` accetta un JSON con:

```json
{
  "action": "toggleFan",
  "params": {}
}
```

Comandi principali:

| Action | Effetto |
| --- | --- |
| `openInternalDoor` | sblocca la porta interna |
| `toggleAlarm` | arma/disarma l'allarme |
| `toggleRfid` | abilita/disabilita lettore RFID |
| `openParkingBarrier` | apre la sbarra parcheggio |
| `vehicleEntered` | incrementa posti occupati e chiude sbarra |
| `vehicleExited` | decrementa posti occupati e chiude sbarra |
| `toggleExteriorLights` | cambia stato luci esterne |
| `toggleAwning` | cambia posizione tenda |
| `syncExteriorAutomation` | luci/tenda seguono `twilightDetected` |
| `toggleFan` | cambia stato ventola |
| `toggleWindows` | cambia stato finestre |
| `syncClimateAutomation` | ventola/finestre seguono temperatura/umidita' |
| `toggleIndoorLights` | cambia luci interne |
| `syncPresenceLighting` | luci interne seguono `motionDetected` |
| `playSelectedBuzzer` | avvia melodia scelta |
| `stopBuzzer` | ferma il buzzer |

Per `playSelectedBuzzer` usa:

```json
{
  "action": "playSelectedBuzzer",
  "params": {
    "melody": "toreador",
    "speed": 125
  }
}
```

Melodie valide:

```text
musicBox
toreador
mega
siren118
doom
```

## Command locks

Quando l'app invia un comando manuale, alcuni campi vengono protetti in
`_commandLocks` per circa 30 secondi. Questo evita che un dispositivo sovrascriva
subito il comando prima di averlo ricevuto.

Esempio: se l'app manda `toggleFan`, `command.php` blocca `fanOn`. Se il bridge
posta subito uno stato vecchio con `fanOn=false`, `device_state.php` lo ignora
finche' il lock scade o finche' arriva lo stato desiderato.

## Installazione locale

Con Laragon:

```text
C:\laragon\www\smart-controller
```

Con XAMPP:

```text
C:\xampp\htdocs\smart-controller
```

Poi apri:

```text
http://127.0.0.1/smart-controller/api/state.php
```

Nell'app Flutter usa come base URL:

```text
http://127.0.0.1/smart-controller
```

## Installazione su Altervista

1. Crea `smart-controller` nella root pubblica del sito.
2. Carica dentro `smart-controller` le cartelle `api` e `storage`.
3. Verifica che `storage/state.json` sia scrivibile dal server.
4. Nell'app usa `https://tuosito.altervista.org/smart-controller`.

Non usare URL con `public_html`. L'endpoint corretto e':

```text
https://tuosito.altervista.org/smart-controller/api/state.php
```

## Test manuali

Leggere lo stato:

```powershell
Invoke-RestMethod "http://127.0.0.1/smart-controller/api/state.php"
```

Inviare un comando:

```powershell
Invoke-RestMethod `
  -Method Post `
  -ContentType "application/json" `
  -Body '{"action":"toggleFan"}' `
  "http://127.0.0.1/smart-controller/api/command.php"
```

Simulare lo stato inviato dal bridge:

```powershell
Invoke-RestMethod `
  -Method Post `
  -ContentType "application/json" `
  -Body '{"temperature":24,"humidity":50,"sensorOk":true,"fanOn":true,"fanPower":160,"fanPwm":160,"windowsOpen":false,"remoteOverride":false}' `
  "http://127.0.0.1/smart-controller/api/device_state.php"
```

## Verifica sintassi PHP

Con Laragon:

```powershell
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\config.php
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\state.php
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\command.php
C:\laragon\bin\php\php-8.3.30-nts-Win32-vs16-x64\php.exe -l server\api\device_state.php
```

## Problemi comuni

### `Invalid JSON`

Il body del POST non e' JSON valido oppure manca `Content-Type:
application/json`.

### L'app vede dati vecchi

Apri `state.php` e controlla `lastUpdated`. Se non cambia, il bridge non sta
postando su `device_state.php` oppure il file `storage/state.json` non e'
scrivibile.

### Comando ignorato dal dispositivo

Controlla che:

- `command.php` risponda `ok: true`;
- lo stato JSON contenga il campo cambiato;
- il bridge riesca a fare `GET state.php`;
- il modulo Arduino riceva una riga `CMD;...`.

### Altervista non salva lo stato

Controlla permessi della cartella `storage` e del file `state.json`. Se serve,
ricarica `state.json` e rendi scrivibile la cartella dal pannello file manager.
