#include "renderer.h"
#include "display.h"
#include "icons.h"
#include "config.h"
#include <time.h>

// Stock Adafruit_GFX FreeFonts (ASCII 0x20-0x7E only — no accents/°).
// Stock set is 9/12/18/24 pt; larger sizes via setTextSize() scaling.
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// Font role -> nearest stock font (see header comment in this reply).
#define F_CITY      (&FreeSansBold24pt7b)   // 24 pt bold
#define F_DATE      (&FreeSans12pt7b)       // ~14 pt
#define F_TEMPBIG   (&FreeSansBold24pt7b)   // scaled x2 -> ~giant
#define F_FEELS     (&FreeSans12pt7b)       // 12 pt
#define F_COND      (&FreeSans12pt7b)       // 12 pt
#define F_LABEL     (&FreeSansBold9pt7b)    // 9 pt bold (was regular -> too
                                            // faint on 3-colour e-paper)
#define F_VALUE     (&FreeSansBold9pt7b)    // 9 pt bold (2-col cells too
                                            // narrow for stock 12/24 pt)
#define F_FDAY      (&FreeSansBold12pt7b)   // ~14 pt bold
#define F_FHILO     (&FreeSans12pt7b)       // 12 pt
#define F_AXIS      (&FreeSans9pt7b)        // ~10 pt
#define F_STATUS    (&FreeSans9pt7b)        // ~10 pt
#define F_UMB       (&FreeSans9pt7b)        // ~10 pt (umbrella caption)

// --- canvas geometry --------------------------------------------------------
static const int MARGIN   = 10;
static const int HEAD_LN  = 60;            // hairline under header
static const int COL_DIV  = 285;           // left | right vertical divider
static const int FC_LN    = 183;           // hairline under forecast
static const int STAT_LN  = 487;           // hairline above status bar
static const int LCX      = 120;           // left-column centre (10..278)

// ---------------------------------------------------------------------------
//  Text helpers — always measure before drawing (no overlaps).
// ---------------------------------------------------------------------------
static uint16_t strW(const GFXfont *f, uint8_t sz, const char *s) {
    display.setFont(f);
    display.setTextSize(sz);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    return w;
}

static void textAt(int x, int yBase, const char *s,
                   const GFXfont *f, uint8_t sz = 1) {
    display.setFont(f);
    display.setTextSize(sz);
    display.setTextColor(COLOR_FG);
    display.setCursor(x, yBase);
    display.print(s);
}

static void textCentered(int cx, int yBase, const char *s,
                         const GFXfont *f, uint8_t sz = 1) {
    textAt(cx - strW(f, sz, s) / 2, yBase, s, f, sz);
}

static void textRight(int xRight, int yBase, const char *s,
                      const GFXfont *f, uint8_t sz = 1) {
    textAt(xRight - strW(f, sz, s), yBase, s, f, sz);
}

// Degree mark: a crisp FILLED ring (FreeFonts have no '\xB0' glyph), raised
// toward the cap-height of the preceding digits. Returns x just past it.
static int degRadius(const GFXfont *f, uint8_t sz) {
    display.setFont(f); display.setTextSize(sz);
    int16_t bx, by; uint16_t bw, bh;
    display.getTextBounds("0", 0, 0, &bx, &by, &bw, &bh);
    return constrain((int)bh / 7, 2, 8);
}
static int degAdvance(const GFXfont *f, uint8_t sz) {
    int r = degRadius(f, sz);
    return (2 + r) + 2 * r + 1;     // leading gap (2+r) scales with font size
}
static int degreeMark(int xRight, int yBase, const GFXfont *f, uint8_t sz) {
    display.setFont(f); display.setTextSize(sz);
    int16_t bx, by; uint16_t bw, bh;
    display.getTextBounds("0", 0, 0, &bx, &by, &bw, &bh);
    int r  = constrain((int)bh / 7, 2, 8);
    int cx = xRight + (2 + r) + r;                 // gap (2+r) -> bigger on big text
    int cy = yBase + by + r;                       // by < 0 -> near glyph top
    display.fillCircle(cx, cy, r, COLOR_FG);
    if (r >= 4) display.fillCircle(cx, cy, r - 2, COLOR_BG);   // hollow ring
    return cx + r + 1;
}

// Integer temperature "NN" + degree (+ optional 'C'), centred on cx.
static void tempCentered(int cx, int yBase, int v,
                         const GFXfont *f, uint8_t sz, bool withC) {
    char num[8];
    snprintf(num, sizeof(num), "%d", v);
    uint16_t nW = strW(f, sz, num);
    int      dW = degAdvance(f, sz);
    uint16_t cW = withC ? strW(f, sz, "C") : 0;
    int total   = (int)nW + dW + (withC ? (int)cW : 0);
    int x       = cx - total / 2;

    textAt(x, yBase, num, f, sz);
    int xa = degreeMark(x + nW, yBase, f, sz);
    if (withC) textAt(xa, yBase, "C", f, sz);
}

static const char *italianDayFull(int wday) {            // ASCII-safe
    static const char *d[7] = {"Domenica","Lunedi","Martedi","Mercoledi",
                               "Giovedi","Venerdi","Sabato"};
    return d[(wday % 7 + 7) % 7];
}

// ---------------------------------------------------------------------------
//  Header: city + date (top-right), hairline
// ---------------------------------------------------------------------------
static void drawHeader(const struct tm &now) {
    textRight(EPD_WIDTH - MARGIN, 34, CITY_NAME, F_CITY);

    char d[64];
    snprintf(d, sizeof(d), "%s, %d %s %d",
             italianDayFull(now.tm_wday), now.tm_mday,
             italianMonth(now.tm_mon + 1), now.tm_year + 1900);
    textRight(EPD_WIDTH - MARGIN, 54, d, F_DATE);

    display.drawLine(MARGIN, HEAD_LN, EPD_WIDTH - MARGIN, HEAD_LN, COLOR_FG);
}

// ---------------------------------------------------------------------------
//  Left column: big temp, feels-like, condition, icon, 8-cell stats grid
// ---------------------------------------------------------------------------
static void statCell(int cx, int cy, StatIcon ic,
                     const char *label, const char *value) {
    drawStatIcon(cx + 4, cy + 9, 18, ic, COLOR_FG);
    textAt(cx + 28, cy + 13, label, F_LABEL);
    textAt(cx + 28, cy + 33, value, F_VALUE);
}

static void drawCurrentConditions(const WeatherData &w) {
    // giant current temperature (BLACK), e.g. "19°C"
    tempCentered(LCX, 150, (int)lroundf(w.temp), F_TEMPBIG, 2, /*withC*/true);

    // "Percepita NN°"
    {
        char b[20];
        snprintf(b, sizeof(b), "Percepita %d", (int)lroundf(w.feelsLike));
        uint16_t bw = strW(F_FEELS, 1, b);
        int dW = degAdvance(F_FEELS, 1);
        int x  = LCX - ((int)bw + dW) / 2;
        textAt(x, 178, b, F_FEELS);
        degreeMark(x + bw, 178, F_FEELS, 1);
    }

    textCentered(LCX, 202, weatherDescription(w.weatherCode), F_COND);

    // current-weather icon (left) and umbrella widget (right) share the band
    drawWeatherIcon(10, 210, 100, iconForCode(w.weatherCode),
                    COLOR_FG, COLOR_ACCENT);

    // umbrella widget — driven by max precip probability over next 12 h
    {
        int popMax = 0, popHour = -1;
        int cnt = w.hourlyCount < 12 ? w.hourlyCount : 12;
        for (int i = 0; i < cnt; ++i) {
            if (w.hourly[i].precipProb > popMax)
                popMax = w.hourly[i].precipProb;
            if (popHour < 0 && w.hourly[i].precipProb >= 70)
                popHour = w.hourly[i].hour;
        }
        bool closed  = (popMax >= 30 && popMax < 70);   // furled umbrella
        bool crossed = (popMax < 30);                   // open + big X

        drawUmbrella(184, 212, 70, closed, crossed, COLOR_FG);

        char u[40];
        if (popMax < 30)
            snprintf(u, sizeof(u), "Niente pioggia (%d%%)", popMax);
        else if (popMax < 70)
            snprintf(u, sizeof(u), "Forse pioggia (%d%%)", popMax);
        else if (popHour >= 0)
            snprintf(u, sizeof(u), "Pioggia ~%02d:00 (%d%%)", popHour, popMax);
        else
            snprintf(u, sizeof(u), "Pioggia (%d%%)", popMax);
        textCentered(184, 300, u, F_UMB);
    }

    // 8-cell stats grid (2 cols x 4 rows), y 326..478
    char sWind[16], sHum[8], sUv[16], sPres[12];
    snprintf(sWind, sizeof(sWind), "%d km/h %s",
             (int)lroundf(w.windSpeed), windCompass(w.windDir));
    snprintf(sHum,  sizeof(sHum),  "%d%%", w.humidity);
    snprintf(sUv,   sizeof(sUv),   "%.0f %s", w.uvIndex,
             uvDescription(w.uvIndex));
    snprintf(sPres, sizeof(sPres), "%d hPa", (int)lroundf(w.pressure));

    const int c0 = 10, c1 = 144, rowH = 38, gy = 326;
    statCell(c0, gy + 0*rowH, StatIcon::Sunrise,    "Alba",       w.sunrise);
    statCell(c1, gy + 0*rowH, StatIcon::Sunset,     "Tramonto",   w.sunset);
    statCell(c0, gy + 1*rowH, StatIcon::Wind,       "Vento",      sWind);
    statCell(c1, gy + 1*rowH, StatIcon::Humidity,   "Umidita",    sHum);
    statCell(c0, gy + 2*rowH, StatIcon::Uv,         "Indice UV",  sUv);
    statCell(c1, gy + 2*rowH, StatIcon::Pressure,   "Pressione",  sPres);
    statCell(c0, gy + 3*rowH, StatIcon::Visibility, "Visibilita", "N/D");
    statCell(c1, gy + 3*rowH, StatIcon::AirQuality, "Qualita Aria","N/D");

    // column divider
    display.drawLine(COL_DIV, HEAD_LN + 4, COL_DIV, STAT_LN - 2, COLOR_FG);
}

// ---------------------------------------------------------------------------
//  Top-right: 5-day forecast row
// ---------------------------------------------------------------------------
static void drawForecast(const WeatherData &w) {
    const int x0 = COL_DIV + 7, x1 = EPD_WIDTH - MARGIN;
    const int colW = (x1 - x0) / FORECAST_DAYS;

    for (int i = 0; i < FORECAST_DAYS; ++i) {
        int cx = x0 + i * colW + colW / 2;
        if (i > 0)
            display.drawLine(x0 + i * colW, HEAD_LN + 6,
                             x0 + i * colW, FC_LN - 4, COLOR_FG);

        textCentered(cx, 84, italianDayShort(w.daily[i].wday), F_FDAY);
        drawWeatherIcon(cx - 24, 94, 48,
                        iconForCode(w.daily[i].weatherCode),
                        COLOR_FG, COLOR_ACCENT);

        // hi/lo: "NN°/NN°"
        int hi = (int)lroundf(w.daily[i].tempMax);
        int lo = (int)lroundf(w.daily[i].tempMin);
        char a[6], b[6];
        snprintf(a, sizeof(a), "%d", hi);
        snprintf(b, sizeof(b), "%d", lo);
        uint16_t aw = strW(F_FHILO, 1, a);
        uint16_t sw = strW(F_FHILO, 1, "/");
        uint16_t bw = strW(F_FHILO, 1, b);
        int dW = degAdvance(F_FHILO, 1);
        int total = (int)aw + dW + (int)sw + (int)bw + dW;
        int x = cx - total / 2;
        textAt(x, 170, a, F_FHILO);
        int xd1 = degreeMark(x + aw, 170, F_FHILO, 1);
        textAt(xd1, 170, "/", F_FHILO);
        int xlo = xd1 + sw;
        textAt(xlo, 170, b, F_FHILO);
        degreeMark(xlo + bw, 170, F_FHILO, 1);
    }

    display.drawLine(COL_DIV + 7, FC_LN, EPD_WIDTH - MARGIN, FC_LN, COLOR_FG);
}

// ---------------------------------------------------------------------------
//  Right-bottom: 24-hour temperature line + precipitation bars
// ---------------------------------------------------------------------------
static void drawOutlookGraph(const WeatherData &w) {
    textAt(COL_DIV + 7, 206, "Temperatura e Pioggia - 24 ore",
           F_AXIS);

    int n = w.hourlyCount;
    const int gx0 = COL_DIV + 51, gx1 = EPD_WIDTH - 58;   // plot x
                                                          // (room for "100%")
    const int gy0 = 224,          gy1 = 452;              // plot y (top..bot)

    if (n < 2) {
        textCentered((gx0 + gx1) / 2, (gy0 + gy1) / 2,
                     "Dati orari non disponibili", F_AXIS);
        return;
    }

    // axes box
    display.drawLine(gx0, gy0, gx0, gy1, COLOR_FG);
    display.drawLine(gx1, gy0, gx1, gy1, COLOR_FG);
    display.drawLine(gx0, gy1, gx1, gy1, COLOR_FG);

    float tmin = w.hourly[0].temp, tmax = w.hourly[0].temp;
    for (int i = 1; i < n; ++i) {
        tmin = min(tmin, w.hourly[i].temp);
        tmax = max(tmax, w.hourly[i].temp);
    }
    if (tmax - tmin < 1.0f) { tmax += 1.0f; tmin -= 1.0f; }

    // inset the plotted data so the Y-axis labels have breathing room
    const int pad = 16;
    int px0 = gx0 + pad, px1 = gx1 - pad;
    auto xAt = [&](int i){ return px0 + (int)((float)i*(px1-px0)/(n-1)); };
    auto yAt = [&](float t){
        return (int)(gy1 - (t - tmin)/(tmax - tmin)*(gy1 - gy0));
    };

    // precipitation bars — hatched (lighter than the solid temp line)
    int barW = max(2, (px1 - px0) / n - 4);
    for (int i = 0; i < n; ++i) {
        int h = (int)(w.hourly[i].precipProb / 100.0f * (gy1 - gy0));
        if (h <= 0) continue;
        int bx = xAt(i) - barW / 2, by = gy1 - h;
        display.drawRect(bx, by, barW, h, COLOR_FG);
        for (int yy = by + 3; yy < gy1; yy += 4)            // hatch
            display.drawLine(bx, yy, bx + barW - 1, yy, COLOR_FG);
    }

    // temperature line — solid, 2 px
    for (int i = 1; i < n; ++i) {
        int xa = xAt(i-1), ya = yAt(w.hourly[i-1].temp);
        int xb = xAt(i),   yb = yAt(w.hourly[i].temp);
        display.drawLine(xa, ya,     xb, yb,     COLOR_FG);
        display.drawLine(xa, ya + 1, xb, yb + 1, COLOR_FG);
    }

    // left Y axis: "NN°C", right-aligned just left of the axis line
    auto drawTempAxis = [&](int v, int yb){
        char num[6]; snprintf(num, sizeof(num), "%d", v);
        int nW = strW(F_AXIS, 1, num);
        int dW = degAdvance(F_AXIS, 1);
        int cW = strW(F_AXIS, 1, "C");
        int xs = (gx0 - 6) - (nW + dW + cW);
        textAt(xs, yb, num, F_AXIS);
        int xa = degreeMark(xs + nW, yb, F_AXIS, 1);
        textAt(xa, yb, "C", F_AXIS);
    };
    drawTempAxis((int)lroundf(tmax), gy0 + 6);
    drawTempAxis((int)lroundf(tmin), gy1);
    textAt(gx1 + 5, gy0 + 6, "100%", F_AXIS);
    textAt(gx1 + 5, gy1,     "0%",   F_AXIS);

    char lbl[8];
    for (int i = 0; i < n; i += 2) {                        // hour labels
        snprintf(lbl, sizeof(lbl), "%02d", w.hourly[i].hour);
        textCentered(xAt(i), gy1 + 18, lbl, F_AXIS);
    }
}

// ---------------------------------------------------------------------------
//  Status bar
// ---------------------------------------------------------------------------
static void drawStatusBar(int rssi, int batteryPct, const struct tm &now) {
    display.drawLine(MARGIN, STAT_LN, EPD_WIDTH - MARGIN, STAT_LN, COLOR_FG);
    int yb = STAT_LN + 25;

    char s[40];
    drawStatIcon(MARGIN + 2, yb - 14, 16, StatIcon::Wifi, COLOR_FG);
    snprintf(s, sizeof(s), "%d dBm", rssi);
    textAt(MARGIN + 24, yb, s, F_STATUS);

    if (batteryPct >= 0) snprintf(s, sizeof(s), "Batteria %d%%", batteryPct);
    else                 snprintf(s, sizeof(s), "Batteria N/D");
    textCentered(EPD_WIDTH / 2, yb, s, F_STATUS);

    snprintf(s, sizeof(s), "Ultimo agg. %02d/%02d %02d:%02d",
             now.tm_mday, now.tm_mon + 1, now.tm_hour, now.tm_min);
    textRight(EPD_WIDTH - MARGIN, yb, s, F_STATUS);
}

// ---------------------------------------------------------------------------
//  Public entry points (signatures unchanged — main.cpp untouched)
// ---------------------------------------------------------------------------
void renderDashboard(const WeatherData &w, int rssi, int batteryPct) {
    time_t t = time(nullptr);
    struct tm now;
    localtime_r(&t, &now);

    Serial.println("[REND] Rendering dashboard (paged)…");
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(COLOR_BG);
        display.setTextSize(1);

        drawHeader(now);
        drawCurrentConditions(w);
        drawForecast(w);
        drawOutlookGraph(w);
        drawStatusBar(rssi, batteryPct, now);
    } while (display.nextPage());
    Serial.println("[REND] Done.");
}

void renderErrorScreen(const char *title, const char *detail) {
    Serial.printf("[REND] Error screen: %s / %s\n", title, detail);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(COLOR_BG);
        display.setTextSize(1);
        textCentered(EPD_WIDTH / 2, EPD_HEIGHT / 2 - 50,
                     "STAZIONE METEO", F_CITY);
        textCentered(EPD_WIDTH / 2, EPD_HEIGHT / 2 + 6,
                     title, &FreeSansBold18pt7b);
        textCentered(EPD_WIDTH / 2, EPD_HEIGHT / 2 + 50,
                     detail, F_DATE);
        char s[64];
        snprintf(s, sizeof(s), "Nuovo tentativo tra %d minuti", SLEEP_MINUTES);
        textCentered(EPD_WIDTH / 2, EPD_HEIGHT / 2 + 96, s, F_STATUS);
    } while (display.nextPage());
}
