#include "display.h"
#include "config.h"
#include <Arduino.h>

// Single global display instance, wired to the fixed pin map.
DisplayType display(GxEPD2_750c_Z90(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void initDisplay() {
    Serial.println("[EPD] Powering driver board…");
    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH);
    delay(100);                       // critical: let the panel power up

    // 50 ms reset duration — at 2 ms the SSD1677 does not come up reliably.
    // initial=true performs the full controller init.
    display.init(115200, true, 50, false);

    // IMPORTANT: do NOT call SPI.end()/SPI.begin() here — display.init()
    // already owns the bus on pins 18/23/19.

    display.setRotation(0);           // 880 wide x 528 tall
    display.setTextWrap(false);
    display.setFullWindow();
    Serial.printf("[EPD] init done: %dx%d\n",
                  display.width(), display.height());
}

void powerOffDisplay() {
    Serial.println("[EPD] Hibernating panel…");
    display.hibernate();              // deep power-down of the controller
    delay(20);
    digitalWrite(EPD_PWR, LOW);       // cut VCC to the driver board
}
