#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <time.h>

#include "config.h"
#include "display.h"
#include "weather_api.h"
#include "renderer.h"

// ---------------------------------------------------------------------------
//  Deep sleep
// ---------------------------------------------------------------------------
static void goToDeepSleep() {
    uint64_t us = (uint64_t)SLEEP_MINUTES * 60ULL * 1000000ULL;
    Serial.printf("[PWR] Deep sleep for %d min (%llu us)\n",
                  SLEEP_MINUTES, us);
    Serial.flush();
    esp_sleep_enable_timer_wakeup(us);
    esp_deep_sleep_start();            // never returns
}

// ---------------------------------------------------------------------------
//  WiFi: WIFI_RETRIES attempts, WIFI_TIMEOUT_MS each
// ---------------------------------------------------------------------------
static bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    for (int attempt = 1; attempt <= WIFI_RETRIES; ++attempt) {
        Serial.printf("[WiFi] Attempt %d/%d -> %s\n",
                      attempt, WIFI_RETRIES, WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED &&
               millis() - start < WIFI_TIMEOUT_MS) {
            delay(250);
            Serial.print('.');
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected: %s  RSSI %d dBm\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());
            return true;
        }
        Serial.println("[WiFi] Attempt failed");
        WiFi.disconnect(true);
        delay(500);
    }
    return false;
}

// ---------------------------------------------------------------------------
//  NTP time sync
// ---------------------------------------------------------------------------
static bool syncTime() {
    Serial.println("[NTP] Syncing time…");
    configTzTime(TIMEZONE_TZ, NTP_SERVER_1, NTP_SERVER_2);

    struct tm tm;
    for (int i = 0; i < 40; ++i) {            // up to ~20 s
        if (getLocalTime(&tm, 100) && tm.tm_year > (2020 - 1900)) {
            Serial.printf("[NTP] %04d-%02d-%02d %02d:%02d:%02d\n",
                          tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                          tm.tm_hour, tm.tm_min, tm.tm_sec);
            return true;
        }
        delay(500);
    }
    Serial.println("[NTP] Sync failed");
    return false;
}

// ---------------------------------------------------------------------------
//  Optional battery reading (returns -1 when disabled / unavailable)
// ---------------------------------------------------------------------------
static int readBatteryPercent() {
#if BATTERY_ENABLED
    analogReadResolution(12);
    uint32_t acc = 0;
    for (int i = 0; i < 16; ++i) acc += analogRead(BATTERY_ADC_PIN);
    float adc = acc / 16.0f;
    float v = (adc / 4095.0f) * 3.3f * BATTERY_DIV_RATIO;
    int pct = (int)((v - BATTERY_V_MIN) /
                    (BATTERY_V_MAX - BATTERY_V_MIN) * 100.0f);
    pct = constrain(pct, 0, 100);
    Serial.printf("[BAT] %.2f V -> %d%%\n", v, pct);
    return pct;
#else
    return -1;
#endif
}

// ---------------------------------------------------------------------------
//  Boot cycle (everything happens once, then deep sleep)
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(50);
    Serial.println("\n=== ESP32 E-Paper Weather Station ===");
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    Serial.printf("[PWR] Wake cause: %d (%s)\n", cause,
                  cause == ESP_SLEEP_WAKEUP_TIMER ? "timer" : "power-on");

    initDisplay();   // EPD_PWR HIGH + delay(100) + init(115200,true,50,false)

    if (!connectWiFi()) {
        renderErrorScreen("WiFi non disponibile",
                          "Controlla SSID e password in config.h");
        powerOffDisplay();
        goToDeepSleep();
        return;
    }

    if (!syncTime()) {
        // Non-fatal: dates may be wrong but weather still useful.
        Serial.println("[NTP] Continuing without confirmed time");
    }

    WeatherData w;
    if (!fetchWeather(w) || !w.valid) {
        renderErrorScreen("Dati meteo non disponibili",
                          "Impossibile contattare Open-Meteo");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        powerOffDisplay();
        goToDeepSleep();
        return;
    }

    int rssi    = WiFi.RSSI();
    int battery = readBatteryPercent();

    renderDashboard(w, rssi, battery);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    powerOffDisplay();
    goToDeepSleep();
}

void loop() {
    // Unused: setup() always ends in deep sleep, which resets into setup().
}
