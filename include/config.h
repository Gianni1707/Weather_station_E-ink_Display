#pragma once

// ===========================================================================
//  USER CONFIGURATION
//  Edit the values in this section, then build & upload.
// ===========================================================================

// --- WiFi credentials & location -------------------------------------------
// These are injected at build time from the (git-ignored) .env file by
// load_env.py (see platformio.ini -> extra_scripts). The placeholders below
// are only fallbacks so the project still compiles without a .env; the
// firmware will fail to connect / show Bari until you create one.
// Copy .env.example to .env and fill in your values.
#ifndef WIFI_SSID
#define WIFI_SSID       "CHANGE_ME"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD   "CHANGE_ME"
#endif

#ifndef LOCATION_LAT
#define LOCATION_LAT     41.1171f
#endif
#ifndef LOCATION_LON
#define LOCATION_LON     16.8719f
#endif
#ifndef CITY_NAME
#define CITY_NAME        "Bari"
#endif

// --- Time -------------------------------------------------------------------
// POSIX TZ string for Europe/Rome (handles CET/CEST DST automatically).
#define TIMEZONE_TZ      "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1     "pool.ntp.org"
#define NTP_SERVER_2     "time.nist.gov"

// --- Power / sleep ----------------------------------------------------------
#define SLEEP_MINUTES    15          // deep-sleep duration between updates

// --- WiFi connection behaviour ---------------------------------------------
#define WIFI_TIMEOUT_MS  60000UL     // 60 s per attempt
#define WIFI_RETRIES     3

// --- Battery monitoring (optional) -----------------------------------------
// Set BATTERY_ENABLED to 1 only if you wired a voltage divider to an ADC pin.
// When 0 the footer simply shows "—" for battery.
#define BATTERY_ENABLED  0
#define BATTERY_ADC_PIN  34
#define BATTERY_DIV_RATIO 2.0f       // (R1+R2)/R2 of your divider
#define BATTERY_V_MIN    3.30f       // empty (0 %)
#define BATTERY_V_MAX    4.20f       // full  (100 %)

// ===========================================================================
//  HARDWARE PIN MAP — DO NOT CHANGE
//  HINK-E075A07-A0 (Good Display GDEH075Z90, SSD1677) on Waveshare HAT.
// ===========================================================================
#define EPD_CS    13
#define EPD_DC    22
#define EPD_RST   21
#define EPD_BUSY  14
#define EPD_PWR   26   // controls VCC of the driver board
// SPI is ESP32 hardware default: SCK=18, MOSI=23, MISO=19 — do NOT remap.

// --- Display geometry -------------------------------------------------------
#define EPD_WIDTH   880
#define EPD_HEIGHT  528

// --- Colours (alias the GxEPD2 macros for readability) ---------------------
#define COLOR_BG     GxEPD_WHITE
#define COLOR_FG     GxEPD_BLACK
#define COLOR_ACCENT GxEPD_RED     // use sparingly
