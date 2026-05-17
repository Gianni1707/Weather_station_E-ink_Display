#include "weather_api.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// ---------------------------------------------------------------------------
//  Open-Meteo request
//  No API key required. timezone=auto -> all times come back as LOCAL time,
//  which is exactly what we want for the display.
// ---------------------------------------------------------------------------
static String buildUrl() {
    String url = "https://api.open-meteo.com/v1/forecast";
    url += "?latitude=";  url += String(LOCATION_LAT, 4);
    url += "&longitude="; url += String(LOCATION_LON, 4);
    url += "&current=temperature_2m,relative_humidity_2m,apparent_temperature,"
           "is_day,weather_code,surface_pressure,wind_speed_10m,wind_direction_10m";
    url += "&hourly=temperature_2m,precipitation_probability";
    url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
           "sunrise,sunset,uv_index_max";
    url += "&timezone=auto";
    url += "&forecast_days=";
    url += String(FORECAST_DAYS);
    return url;
}

// Extract "HH:MM" out of an ISO string like "2026-05-16T05:42".
static void copyHHMM(const char *iso, char *dst /* [6] */) {
    if (iso && strlen(iso) >= 16) {
        dst[0] = iso[11]; dst[1] = iso[12];
        dst[2] = ':';
        dst[3] = iso[14]; dst[4] = iso[15];
        dst[5] = '\0';
    } else {
        strcpy(dst, "--:--");
    }
}

// Parse the hour field (positions 11..12) of an ISO timestamp.
static int isoHour(const char *iso) {
    if (iso && strlen(iso) >= 13)
        return (iso[11] - '0') * 10 + (iso[12] - '0');
    return -1;
}

bool fetchWeather(WeatherData &out) {
    out.valid = false;

    WiFiClientSecure client;
    client.setInsecure();              // Open-Meteo cert not pinned (v1)
    client.setTimeout(15);

    HTTPClient http;
    String url = buildUrl();
    Serial.printf("[API] GET %s\n", url.c_str());

    if (!http.begin(client, url)) {
        Serial.println("[API] http.begin() failed");
        return false;
    }
    http.useHTTP10(true);              // lets ArduinoJson stream the body

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[API] HTTP error: %d\n", code);
        http.end();
        return false;
    }

    // Filter the response so we only allocate what we use.
    JsonDocument filter;
    filter["current"] = true;
    filter["hourly"]["time"] = true;
    filter["hourly"]["temperature_2m"] = true;
    filter["hourly"]["precipitation_probability"] = true;
    filter["daily"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(
        doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (err) {
        Serial.printf("[API] JSON parse failed: %s\n", err.c_str());
        return false;
    }

    // --- current ----------------------------------------------------------
    JsonObject cur = doc["current"];
    out.temp        = cur["temperature_2m"]        | 0.0f;
    out.feelsLike   = cur["apparent_temperature"]  | out.temp;
    out.weatherCode = cur["weather_code"]          | 0;
    out.humidity    = cur["relative_humidity_2m"]  | 0;
    out.pressure    = cur["surface_pressure"]      | 0.0f;
    out.windSpeed   = cur["wind_speed_10m"]        | 0.0f;
    out.windDir     = cur["wind_direction_10m"]    | 0;
    out.isDay       = (int)(cur["is_day"] | 1) != 0;

    // --- daily ------------------------------------------------------------
    JsonObject daily = doc["daily"];
    JsonArray  dWc   = daily["weather_code"];
    JsonArray  dMax  = daily["temperature_2m_max"];
    JsonArray  dMin  = daily["temperature_2m_min"];
    JsonArray  dUv   = daily["uv_index_max"];
    JsonArray  dSr   = daily["sunrise"];
    JsonArray  dSs   = daily["sunset"];

    out.uvIndex = dUv.isNull() ? 0.0f : (float)(dUv[0] | 0.0f);
    copyHHMM(dSr.isNull() ? nullptr : (const char *)dSr[0], out.sunrise);
    copyHHMM(dSs.isNull() ? nullptr : (const char *)dSs[0], out.sunset);

    // tm_wday for "today": Open-Meteo daily[0] is today.
    time_t nowT = time(nullptr);
    struct tm nowTm;
    localtime_r(&nowT, &nowTm);

    for (int i = 0; i < FORECAST_DAYS; ++i) {
        out.daily[i].weatherCode = dWc[i]  | 0;
        out.daily[i].tempMax     = dMax[i] | 0.0f;
        out.daily[i].tempMin     = dMin[i] | 0.0f;
        out.daily[i].wday        = (nowTm.tm_wday + i) % 7;
    }

    // --- hourly: keep 24 points starting at the current hour --------------
    JsonObject hourly = doc["hourly"];
    JsonArray  hTime  = hourly["time"];
    JsonArray  hTemp  = hourly["temperature_2m"];
    JsonArray  hPrec  = hourly["precipitation_probability"];

    int total   = hTime.size();
    int nowHour = nowTm.tm_hour;

    // Find the first index at/after the current local hour.
    int start = 0;
    for (int i = 0; i < total; ++i) {
        if (isoHour(hTime[i]) == nowHour) { start = i; break; }
    }

    int n = 0;
    for (int i = start; i < total && n < HOURLY_POINTS; ++i, ++n) {
        out.hourly[n].hour       = isoHour(hTime[i]);
        out.hourly[n].temp       = hTemp[i] | 0.0f;
        out.hourly[n].precipProb = hPrec[i] | 0;
        out.hourly[n].isNow      = (n == 0);
    }
    out.hourlyCount = n;

    out.valid = true;
    Serial.printf("[API] OK: %.1f°C, code %d, %d hourly pts\n",
                  out.temp, out.weatherCode, out.hourlyCount);
    return true;
}

// ---------------------------------------------------------------------------
//  Localisation helpers (Italian)
// ---------------------------------------------------------------------------
const char *italianDayShort(int wday) {
    static const char *d[7] = {"Dom","Lun","Mar","Mer","Gio","Ven","Sab"};
    return d[(wday % 7 + 7) % 7];
}

const char *italianMonth(int m) {
    static const char *mo[12] = {
        "Gennaio","Febbraio","Marzo","Aprile","Maggio","Giugno",
        "Luglio","Agosto","Settembre","Ottobre","Novembre","Dicembre"};
    if (m < 1 || m > 12) return "";
    return mo[m - 1];
}

const char *windCompass(int deg) {
    static const char *c[8] = {"N","NE","E","SE","S","SO","O","NO"};
    int idx = (int)((deg + 22.5f) / 45.0f) & 7;
    return c[idx];
}

const char *uvDescription(float uv) {
    if (uv < 3)  return "basso";
    if (uv < 6)  return "moderato";
    if (uv < 8)  return "alto";
    if (uv < 11) return "molto alto";
    return "estremo";
}

// WMO weather codes -> Italian text.
const char *weatherDescription(int code) {
    switch (code) {
        case 0:  return "Sereno";
        case 1:  return "Prev. sereno";
        case 2:  return "Parz. nuvoloso";
        case 3:  return "Nuvoloso";
        case 45:
        case 48: return "Nebbia";
        case 51:
        case 53:
        case 55: return "Pioviggine";
        case 56:
        case 57: return "Piogg. gelata";
        case 61:
        case 63:
        case 65: return "Pioggia";
        case 66:
        case 67: return "Piogg. gelata";
        case 71:
        case 73:
        case 75: return "Neve";
        case 77: return "Granuli neve";
        case 80:
        case 81:
        case 82: return "Rovesci";
        case 85:
        case 86: return "Rovesci neve";
        case 95: return "Temporale";
        case 96:
        case 99: return "Temp. grandine";
        default: return "N/D";
    }
}
