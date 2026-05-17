#pragma once

// GxEPD2 must be configured for the exact panel:
// HINK-E075A07-A0 == Good Display GDEH075Z90, 880x528, 3-colour, SSD1677.
#include <GxEPD2_3C.h>

// 3-colour buffer at 880x528 will not fit in RAM as one piece, so we render
// in 4 vertical pages (HEIGHT / 4).
using DisplayType =
    GxEPD2_3C<GxEPD2_750c_Z90, GxEPD2_750c_Z90::HEIGHT / 4>;

extern DisplayType display;

// Powers the driver board, runs display.init() with the SSD1677-safe
// reset timing, and prepares a full-window paged refresh.
void initDisplay();

// Hibernates the panel and cuts VCC to the driver board before deep sleep.
void powerOffDisplay();
