# ESP32 E-Paper Weather Station

Wakes from deep sleep every 30 minutes, connects to WiFi, syncs time over NTP,
fetches weather from **Open-Meteo** (no API key), renders an Italian weather
dashboard on a **7.5" 880×528 3-colour** e-paper panel, then deep-sleeps again.

- **MCU:** ESP32 DevKit (`esp32dev`)
- **Display:** HINK-E075A07-A0 = Good Display **GDEH075Z90**, 880×528,
  Black/White/Red, SSD1677 — GxEPD2 class `GxEPD2_750c_Z90`
- **Driver board:** Waveshare e-Paper Driver HAT (4-line SPI)

## Wiring

| Signal     | ESP32 GPIO | Notes                                   |
|------------|-----------|------------------------------------------|
| EPD_CS     | 13        | SPI chip select                          |
| EPD_DC     | 22        | Data/Command                             |
| EPD_RST    | 21        | Reset                                    |
| EPD_BUSY   | 14        | Busy (input)                             |
| EPD_PWR    | 26        | Drives VCC of the driver board           |
| SPI SCK    | 18        | ESP32 hardware SPI default — **do not remap** |
| SPI MOSI   | 23        | ESP32 hardware SPI default — **do not remap** |
| SPI MISO   | 19        | ESP32 hardware SPI default — **do not remap** |
| GND / 3V3  | GND / 3V3 | Power the HAT from 3V3                    |

The firmware holds `EPD_PWR` HIGH, waits 100 ms for the panel to power up,
then calls `display.init(115200, true, 50, false)` (50 ms reset — the SSD1677
does not come up reliably at 2 ms). It never calls `SPI.end()/begin()` after
`init()`. CPU stays at the default **240 MHz**; do **not** add
`board_build.f_cpu` (80 MHz silently breaks SSD1677 refreshes).

## Set WiFi credentials & location

WiFi credentials and location live in a **`.env`** file that is **git-ignored**
(never committed). Copy the template and fill in your values:

```bash
cp .env.example .env      # Windows: copy .env.example .env
```

```ini
WIFI_SSID=YOUR_WIFI_SSID
WIFI_PASSWORD=YOUR_WIFI_PASSWORD
LOCATION_LAT=41.1171      # default: Bari, Italy
LOCATION_LON=16.8719
CITY_NAME=Bari
```

At build time `load_env.py` (a PlatformIO `pre:` script wired in
`platformio.ini`) reads `.env` and injects each value as a `-D` compiler
macro. If `.env` is missing the build still succeeds using the placeholder
fallbacks in `include/config.h` (the firmware then can't connect until you
create one).

The remaining, non-secret settings stay in `include/config.h`: timezone /
NTP servers, `SLEEP_MINUTES` (deep-sleep duration, default 30), WiFi
retry/timeout, and battery options. Battery monitoring is **off** by default
(footer shows `Batteria N/D`); set `BATTERY_ENABLED 1` and the divider
constants only if you wired an ADC divider.

## Build & upload

Requires [PlatformIO](https://platformio.org/) (`pip install platformio` or the
VS Code extension).

```bash
cd weather-epd
pio run                 # compile (uses -Wall, builds clean)
pio run -t upload       # flash over USB
pio device monitor -b 115200
```

## Expected serial log (first boot)

```
=== ESP32 E-Paper Weather Station ===
[PWR] Wake cause: 0 (power-on)
[EPD] Powering driver board…
[EPD] init done: 880x528
[WiFi] Attempt 1/3 -> YOUR_WIFI_SSID
.......
[WiFi] Connected: 192.168.1.42  RSSI -52 dBm
[NTP] Syncing time…
[NTP] 2026-05-16 14:30:05
[API] GET https://api.open-meteo.com/v1/forecast?latitude=41.1171&...
[API] OK: 24.3°C, code 1, 24 hourly pts
[REND] Rendering dashboard (paged)…
_PowerOn : 0 : ... us
_Update_Full : 1 : 25xxxxxx us      <-- ~25 s, this is normal for a full refresh
[REND] Done.
[EPD] Hibernating panel…
[PWR] Deep sleep for 30 min (1800000000 us)
```

A full refresh physically takes ~25 s and the `_Update_Full :` line prints a
large microsecond value (~25,000,000). That is expected for a 3-colour panel.

## Layout

880×528, four regions: header (icon + huge red temperature + city/date/sun/
humidity/pressure/wind/UV), 5-day forecast, 24 h temperature + precipitation
graph (red "now" marker), and a footer (WiFi dBm / battery / last update).
Red is used sparingly: city name, current temperature, and the "now" marker.

## Project structure

```
weather-epd/
├── platformio.ini
├── include/config.h
├── src/
│   ├── main.cpp          # boot → WiFi → NTP → fetch → render → sleep
│   ├── display.cpp/.h    # init / power-off, GxEPD2 instance
│   ├── weather_api.cpp/.h# Open-Meteo HTTP + JSON, IT localisation
│   ├── renderer.cpp/.h   # all draw* functions
│   └── icons.cpp/.h       # geometric weather icons
└── README.md
```

## Known limitations

- **Icons are geometric** (drawn with circles/lines), not bitmaps — v1 only.
- **No battery hardware** assumed by default; reading is a stub until you
  enable and wire a divider (`BATTERY_ENABLED`).
- **TLS is not verified** (`client.setInsecure()`) — fine for public,
  read-only Open-Meteo data; not suitable for sensitive endpoints.
- **Full refresh only** (no partial update) — the panel flashes ~25 s each
  cycle; that is inherent to 3-colour SSD1677 panels.
- FreeFonts are ASCII-only, so accented Italian letters are rendered without
  accents (e.g. "Lunedi", "Umidita") and `°` is drawn as a small ring.
- Day-of-week for the forecast is derived as `today + i` (Open-Meteo `daily[0]`
  is today); correct as long as NTP time synced.
```
