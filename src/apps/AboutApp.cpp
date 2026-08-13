#include "AboutApp.h"
#include "../core/AppManager.h"
#include "../core/Config.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/PoppySprites.h"
#include "../ui/assets/Icons.h"
#include <Arduino.h>

const char* const* AboutApp::icon() const { return icons::INFO; }

void AboutApp::update(uint32_t dtMs) {
    // La RAM libre y el uptime cambian: refrescamos 1 vez por segundo.
    _accum += dtMs;
    if (_accum >= 1000) {
        _accum = 0;
        requestRedraw();
    }
}

void AboutApp::draw(M5Canvas& c) {
    using namespace theme;

    pixelart::draw(c, sprites::RABBIT_IDLE, sprites::RABBIT_H, 16, 40, 3);

    c.setFont(&fonts::Font0);
    c.setTextDatum(textdatum_t::top_left);
    const int x = 84;

    c.setTextSize(2);
    c.setTextColor(PRIMARY);
    c.drawString(config::OS_NAME, x, 28);

    c.setTextSize(1);
    char line[48];
    snprintf(line, sizeof(line), "v%s", config::OS_VERSION);
    c.setTextColor(POPPY);
    c.drawString(line, x, 48);

    c.setTextColor(GRAY);
    c.drawString("M5Stack Cardputer ADV", x, 64);
    c.drawString("ESP32-S3 @ 240MHz", x, 76);

    snprintf(line, sizeof(line), "RAM libre: %u KB", (unsigned)(ESP.getFreeHeap() / 1024));
    c.setTextColor(STEM);
    c.drawString(line, x, 92);

    const uint32_t upMin = millis() / 60000;
    snprintf(line, sizeof(line), "Uptime: %uh %02um", (unsigned)(upMin / 60), (unsigned)(upMin % 60));
    c.setTextColor(GRAY);
    c.drawString(line, x, 104);
}

void AboutApp::onKey(const KeyEvent& e) {
    if (e.key == Key::Back) appManager.goBack();
}
