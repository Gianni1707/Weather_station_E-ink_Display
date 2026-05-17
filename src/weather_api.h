#pragma once
#include <Arduino.h>

// Number of forecast days and hourly points we keep.
constexpr int FORECAST_DAYS = 5;
constexpr int HOURLY_POINTS = 24;

struct DailyForecast {
    int     weatherCode = 0;
    float   tempMax     = 0;
    float   tempMin     = 0;
    int     wday        = 0;   // tm_wday: 0=Sunday … 6=Saturday
};

struct HourlyPoint {
    int     hour        = 0;   // 0..23 local
    float   temp        = 0;
    int     precipProb  = 0;   // %
    bool    isNow       = false;
};

struct WeatherData {
    bool    valid       = false;

    // Current conditions
    float   temp        = 0;
    float   feelsLike    = 0;
    int     weatherCode  = 0;
    int     humidity     = 0;   // %
    float   pressure     = 0;   // hPa
    float   windSpeed    = 0;   // km/h
    int     windDir      = 0;   // degrees
    float   uvIndex      = 0;
    bool    isDay        = true;

    // Sun times (local "HH:MM")
    char    sunrise[6]   = "--:--";
    char    sunset[6]    = "--:--";

    DailyForecast daily[FORECAST_DAYS];
    HourlyPoint   hourly[HOURLY_POINTS];
    int           hourlyCount = 0;
};

// Fetches and parses Open-Meteo data into `out`.
// Returns true on success; sets out.valid accordingly.
bool fetchWeather(WeatherData &out);

// --- helpers shared with the renderer --------------------------------------
const char *italianDayShort(int wday);     // "Dom".."Sab"
const char *italianMonth(int month1to12);  // "Gennaio".."Dicembre"
const char *windCompass(int degrees);      // "N","NE","E","SE","S","SO","O","NO"
const char *uvDescription(float uv);       // "basso","moderato",...
const char *weatherDescription(int code);  // Italian condition text
