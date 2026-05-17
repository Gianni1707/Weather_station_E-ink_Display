#pragma once
#include "weather_api.h"

// Draws the full weather dashboard (paged full-window refresh).
// `rssi` = WiFi signal in dBm, `batteryPct` = 0..100 or -1 when unavailable.
void renderDashboard(const WeatherData &w, int rssi, int batteryPct);

// Draws a centred Italian error screen (used when WiFi/API fail).
void renderErrorScreen(const char *title, const char *detail);
