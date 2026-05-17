#include "icons.h"
#include "display.h"
#include "config.h"   // COLOR_BG

// ---------------------------------------------------------------------------
//  WMO weather_code -> icon family
// ---------------------------------------------------------------------------
IconType iconForCode(int code) {
    switch (code) {
        case 0:                       return IconType::Sunny;
        case 1: case 2:               return IconType::PartlyCloudy;
        case 3:                       return IconType::Cloudy;
        case 45: case 48:             return IconType::Fog;
        case 51: case 53: case 55:
        case 56: case 57:
        case 61: case 63: case 65:
        case 66: case 67:
        case 80: case 81: case 82:    return IconType::Rain;
        case 71: case 73: case 75:
        case 77: case 85: case 86:    return IconType::Snow;
        case 95: case 96: case 99:    return IconType::Thunderstorm;
        default:                      return IconType::Cloudy;
    }
}

// A puffy cloud built from circles + a flat base, sized to `w`.
static void cloud(int cx, int cy, int w, uint16_t color) {
    int r = w / 4;
    display.fillCircle(cx - r,           cy,            r,            color);
    display.fillCircle(cx + r,           cy,            (int)(r*0.9), color);
    display.fillCircle(cx,               cy - r/2,      (int)(r*1.2), color);
    display.fillRect  (cx - r - r/2, cy, 3*r, r,                      color);
    // flat underside
    display.fillRect  (cx - r - r/2, cy + r, 3*r, 2, color);
}

static void sun(int cx, int cy, int r, uint16_t c) {
    display.fillCircle(cx, cy, r, c);
    int rayIn = r + r / 3;
    int rayOut = r + r;
    for (int a = 0; a < 360; a += 45) {
        float t = a * 0.01745329f;
        display.drawLine(cx + cosf(t) * rayIn,  cy + sinf(t) * rayIn,
                         cx + cosf(t) * rayOut, cy + sinf(t) * rayOut, c);
        display.drawLine(cx + cosf(t) * rayIn + 1,  cy + sinf(t) * rayIn,
                         cx + cosf(t) * rayOut + 1, cy + sinf(t) * rayOut, c);
    }
}

static void raindrops(int cx, int topY, int w, int n, uint16_t c) {
    int step = w / (n + 1);
    for (int i = 1; i <= n; ++i) {
        int x = cx - w / 2 + i * step;
        display.drawLine(x,     topY,     x - w / 12, topY + w / 5, c);
        display.drawLine(x + 1, topY,     x - w / 12 + 1, topY + w / 5, c);
    }
}

void drawWeatherIcon(int x, int y, int s,
                     IconType type, uint16_t black, uint16_t sunC) {
    int cx = x + s / 2;
    int cy = y + s / 2;

    switch (type) {
        case IconType::Sunny:
            sun(cx, cy, s / 4, sunC);
            break;

        case IconType::PartlyCloudy:
            // sun upper-left (red), cloud lower-right (black) overlapping it
            sun(cx - s / 5, cy - s / 5, s / 7, sunC);
            cloud(cx + s / 10, cy + s / 8, (int)(s * 0.62f), black);
            break;

        case IconType::Cloudy:
            cloud(cx, cy, (int)(s * 0.7f), black);
            break;

        case IconType::Rain:
            cloud(cx, cy - s / 6, (int)(s * 0.62f), black);
            raindrops(cx, cy + s / 6, (int)(s * 0.55f), 3, black);
            break;

        case IconType::Thunderstorm: {
            cloud(cx, cy - s / 6, (int)(s * 0.62f), black);
            int a = s / 12;
            display.fillTriangle(cx - a, cy,           cx + a, cy,
                                 cx - a/2, cy + 2*a,    black);
            display.fillTriangle(cx, cy + a,            cx + 2*a, cy + a,
                                 cx - a, cy + s / 3,    black);
            break;
        }

        case IconType::Snow: {
            cloud(cx, cy - s / 6, (int)(s * 0.62f), black);
            int r = s / 12;
            for (int i = -1; i <= 1; ++i) {
                int fx = cx + i * (s / 5);
                int fy = cy + s / 5;
                display.drawLine(fx - r, fy,     fx + r, fy,     black);
                display.drawLine(fx,     fy - r, fx,     fy + r, black);
                display.drawLine(fx - r, fy - r, fx + r, fy + r, black);
                display.drawLine(fx - r, fy + r, fx + r, fy - r, black);
            }
            break;
        }

        case IconType::Fog: {
            // three horizontal wavy lines, no cloud
            int amp = max(2, s / 24);
            for (int row = 0; row < 3; ++row) {
                int yy = cy - s / 6 + row * (s / 6);
                int prevx = x + s / 8, prevy = yy;
                for (int xx = x + s / 8; xx <= x + s - s / 8; xx += 4) {
                    int wy = yy + (int)(sinf(xx * 0.4f) * amp);
                    display.drawLine(prevx, prevy, xx, wy, black);
                    prevx = xx; prevy = wy;
                }
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
//  Small stats-grid / status-bar glyphs (~16 px, all `c`)
// ---------------------------------------------------------------------------
void drawStatIcon(int x, int y, int s, StatIcon ic, uint16_t c) {
    int cx = x + s / 2;
    int cy = y + s / 2;
    int r  = s / 3;

    switch (ic) {
        case StatIcon::Sunrise:
        case StatIcon::Sunset: {
            // half-sun on the horizon + direction arrow
            int hy = y + s - 3;
            display.drawLine(x, hy, x + s, hy, c);
            for (int a = 200; a <= 340; a += 35) {            // upper arc rays
                float t = a * 0.01745329f;
                display.drawLine(cx + cosf(t) * (r + 1), hy + sinf(t) * (r + 1),
                                 cx + cosf(t) * (r + 4), hy + sinf(t) * (r + 4), c);
            }
            display.drawCircle(cx, hy, r, c);
            if (ic == StatIcon::Sunrise) {
                display.drawLine(cx, y, cx, y + 5, c);
                display.drawLine(cx, y, cx - 3, y + 3, c);
                display.drawLine(cx, y, cx + 3, y + 3, c);
            } else {
                display.drawLine(cx, y, cx, y + 5, c);
                display.drawLine(cx, y + 5, cx - 3, y + 2, c);
                display.drawLine(cx, y + 5, cx + 3, y + 2, c);
            }
            break;
        }

        case StatIcon::Wind:
            for (int i = -1; i <= 1; ++i) {
                int yy = cy + i * 4;
                display.drawLine(x + 1, yy, x + s - 4, yy, c);
                display.drawCircle(x + s - 3, yy, 2, c);
            }
            break;

        case StatIcon::Humidity:
            display.fillTriangle(cx, y + 1, cx - r, cy + 1, cx + r, cy + 1, c);
            display.fillCircle(cx, cy + 2, r, c);
            display.fillCircle(cx - r/2, cy, 1, COLOR_BG);    // highlight
            break;

        case StatIcon::Uv:
            sun(cx, cy, s / 5, c);
            break;

        case StatIcon::Pressure:
            display.drawCircle(cx, cy, r, c);
            display.drawLine(cx, cy, cx + r - 2, cy - r + 2, c);   // needle
            display.fillCircle(cx, cy, 1, c);
            break;

        case StatIcon::Visibility:
            // eye: two arcs + pupil
            display.drawLine(cx - r, cy, cx + r, cy, c);
            display.drawCircle(cx, cy + r, r + 1, c);
            display.drawCircle(cx, cy - r, r + 1, c);
            display.fillCircle(cx, cy, 2, c);
            break;

        case StatIcon::AirQuality:
            display.drawCircle(cx, cy, r, c);
            display.fillCircle(cx - 2, cy, 1, c);
            display.fillCircle(cx + 2, cy - 1, 1, c);
            display.fillCircle(cx + 1, cy + 2, 1, c);
            break;

        case StatIcon::Wifi:
            for (int rr = r; rr >= r - 4 && rr > 0; rr -= 3)
                for (int a = 215; a <= 325; a += 6) {
                    float t = a * 0.01745329f;
                    display.drawPixel(cx + cosf(t) * rr,
                                      y + s - 2 + sinf(t) * rr, c);
                }
            display.fillCircle(cx, y + s - 2, 1, c);
            break;
    }
}

// ---------------------------------------------------------------------------
//  Umbrella widget (primitives only)
// ---------------------------------------------------------------------------
void drawUmbrella(int cx, int topY, int s,
                  bool closed, bool crossed, uint16_t c) {
    int rc        = (int)(s * 0.42f);          // canopy radius
    int domeY     = topY + rc;                 // flat bottom of open canopy
    int handleEnd = topY + s - 4;

    if (!closed) {
        display.drawLine(cx, topY - 3, cx, topY + 1, c);          // finial
        display.fillCircle(cx, domeY, rc, c);                     // canopy
        display.fillRect(cx - rc - 1, domeY + 1,
                         2 * rc + 2, rc + 3, COLOR_BG);           // cut bottom
        display.drawLine(cx - rc, domeY, cx + rc, domeY, c);      // rim
    } else {
        // furled / closed umbrella: slim tapered body
        display.fillTriangle(cx, topY - 2, cx - rc / 2, domeY,
                             cx + rc / 2, domeY, c);
        display.fillTriangle(cx - rc / 2, domeY, cx + rc / 2, domeY,
                             cx, domeY + rc, c);
        domeY += rc;                                              // shaft lower
    }

    display.drawLine(cx,     domeY, cx,     handleEnd, c);        // shaft (2px)
    display.drawLine(cx + 1, domeY, cx + 1, handleEnd, c);

    int hk = max(4, (int)(s * 0.12f));                            // J-hook
    display.drawLine(cx,      handleEnd, cx - hk, handleEnd,      c);
    display.drawLine(cx + 1,  handleEnd, cx - hk, handleEnd,      c);
    display.drawLine(cx - hk, handleEnd, cx - hk, handleEnd - hk, c);

    if (crossed) {                       // big X spanning the whole widget
        int xL = cx - rc - 12, xR = cx + rc + 12;
        int yT = topY - 6,     yB = handleEnd;
        for (int o = -1; o <= 1; ++o) {                           // ~3 px thick
            display.drawLine(xL, yT + o, xR, yB + o, c);
            display.drawLine(xL, yB + o, xR, yT + o, c);
        }
    }
}
