# Luci interne

Modulo non ancora implementato. Questa cartella contiene solo la documentazione
del comportamento previsto, cosi' il contratto con app, server e bridge resta
chiaro quando verra' scritto lo sketch.

## Funzioni previste

- rilevare presenza o movimento con sensore a ultrasuoni, PIR o altro sensore;
- accendere/spegnere luci interne;
- inviare lo stato al bridge ESP32;
- ricevere comandi manuali dall'app;
- eventualmente tornare automatico dopo un timeout, come `esterni_tenda`.

## File previsto

```text
firmware/luci_interne/luci_interne.ino
```

## Collegamento previsto

Se verra' usato Arduino Uno:

```text
ESP32 TX2 GPIO17 -> Arduino RX pin 0
Arduino TX pin 1 -> partitore 1k/2k -> ESP32 RX2 GPIO16
GND ESP32 -> GND Arduino
```

Se verra' usato Arduino Mega:

```text
ESP32 TX2 GPIO17 -> Mega RX1 pin 19
Mega TX1 pin 18 -> partitore 1k/2k -> ESP32 RX2 GPIO16
GND ESP32 -> GND Mega
```

## Stato da inviare

Lo sketch dovrebbe inviare almeno:

```text
STATE;motionDetected=1;indoorLightsOn=1;
```

Campi:

| Campo | Significato |
| --- | --- |
| `motionDetected` | presenza/movimento rilevato |
| `indoorLightsOn` | luci interne accese |

## Comandi da leggere

Lo sketch dovrebbe leggere:

```text
CMD;indoorLightsOn=1;
```

L'app ha gia' i comandi:

```text
toggleIndoorLights
syncPresenceLighting
```

`syncPresenceLighting` imposta `indoorLightsOn = motionDetected` lato server.

## Suggerimento per lo sketch

Una struttura coerente con gli altri moduli:

```text
LINK_BAUD = 9600 o 57600
SEND_STATE_MS = 1000
COMMAND_TIMEOUT_MS = 10000
DEBUG_STATUS_MS = 10000
```

Funzioni consigliate:

```text
readSerialCommands()
sendStateToEsp32()
applyCommandLine()
updatePresenceAutomation()
```

## Quando sara' completo

Aggiornare questo README con:

- scheda scelta;
- pin definitivi;
- librerie richieste;
- soglie sensore;
- protocollo `STATE;...`;
- protocollo `CMD;...`;
- procedura di test.
