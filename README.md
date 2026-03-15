t# SmallTV RSS

Firmware ESP8266 per GeekMagic SmallTV con:
- orologio NTP
- meteo OpenWeatherMap
- news RSS ANSA
- GTT (web + TFT)
- OTA via ElegantOTA

## Versioning

- Versione attuale: `2026.03.15`
- Formato versione: `YYYY.MM.DD`

### Changelog breve

- `2026.03.15`: pagina GTT esterna in `gtt_html.h`, fallback GTT unificato (web + TFT), hardening RAM/payload.
- `2026.03.11`: refresh UI/typography.

## Requisiti

- ESP8266 (es. D1 mini / ESP-12)
- ST7789 240x240
- WiFi 2.4 GHz
- Arduino IDE o `arduino-cli`

## Setup rapido

1. Clona il progetto.
2. Crea `wifi_secrets.h` da `wifi_secrets.example.h`.
3. Crea `secrets.h` da `secrets.example.h` e inserisci `OWM_API_KEY`.
4. Compila e carica `SmallTV_RSS.ino`.

## Endpoints

### Pagine HTML

- `GET /` dashboard (`index_html.h`)
- `GET /gtt` pagina GTT (`gtt_html.h`)
- `GET /update` OTA (ElegantOTA)

### API e controllo

- `GET /api` stato dispositivo + `fw_version`
- `GET /news` news ANSA + debug
- `GET /gtt_data` GTT + debug
- `GET /brightness?value=N` luminosita' (`0..255`)

## Fallback GTT

Se il feed GTT non e' disponibile, firmware e web usano lo stesso dataset di fallback:
- linee `15`, `16`, `33`
- usato sia su TFT che su `/gtt_data`

## Note tecniche

- HTML in PROGMEM: `index_html.h`, `gtt_html.h`
- limiti payload:
  - meteo `4 KB`
  - GTT `4 KB`
  - RSS `32 KB`

## Build

```bash
arduino-cli compile --fqbn esp8266:esp8266:d1_mini SmallTV_RSS.ino
```

## Licenza

MIT (`LICENSE`).
