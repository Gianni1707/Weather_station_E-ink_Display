#pragma once
#include <stdint.h>

// Weather icon families (mapped from Open-Meteo WMO codes).
enum class IconType {
    Sunny,
    PartlyCloudy,
    Cloudy,
    Rain,
    Thunderstorm,
    Snow,
    Fog
};

// Small monochrome icons used in the left-column stats grid / status bar.
enum class StatIcon {
    Sunrise,
    Sunset,
    Wind,
    Humidity,
    Uv,
    Pressure,
    Visibility,
    AirQuality,
    Wifi
};

IconType iconForCode(int weatherCode);

// Draws a weather icon inside the box (x, y, size, size). Everything is drawn
// in `black`; only the sun disc/rays use `sun` (the single red accent).
// Scales cleanly for 48x48 (forecast) and 128x128 (current).
void drawWeatherIcon(int x, int y, int size,
                     IconType type, uint16_t black, uint16_t sun);

// Draws a ~16x16 stat glyph, all in `color` (black).
void drawStatIcon(int x, int y, int size, StatIcon ic, uint16_t color);

// WiFi status icon inside an (x, y, size, size) box: upward signal arcs +
// node dot. When `connected` is false a diagonal slash is drawn over it
// ("no connection"). Everything in `color`.
void drawWifiIcon(int x, int y, int size, bool connected, uint16_t color);

// Umbrella widget centred on `cx`, top at `topY`, ~`size` tall (all `color`).
//   closed  = furled umbrella (used for "maybe rain")
//   crossed = open umbrella with a big diagonal X (used for "no rain")
void drawUmbrella(int cx, int topY, int size,
                  bool closed, bool crossed, uint16_t color);
