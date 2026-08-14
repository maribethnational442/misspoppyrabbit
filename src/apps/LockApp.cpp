#include "LockApp.h"
#include <ctime>
#include "../core/Lang.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/Icons.h"
#include "../ui/assets/PoppySprites.h"

LockApp lockApp;

void LockApp::update(uint32_t dtMs) {
    (void)dtMs;
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    if (t.tm_min != _lastMin) {
        _lastMin = t.tm_min;
        requestRedraw();
    }
}

void LockApp::draw(M5Canvas& c) {
    using namespace theme;

    // Conejo dormido (el fotograma de ojos cerrados, para eso estaba)
    pixelart::draw(c, sprites::RABBIT_BLINK, sprites::RABBIT_H, 24, 44, 3);

    c.setFont(&fonts::Font0);
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    if (t.tm_year + 1900 >= 2020) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        c.setTextDatum(textdatum_t::middle_left);
        c.setTextSize(4);
        c.setTextColor(PRIMARY);
        c.drawString(buf, 96, 66);
    }

    pixelart::draw(c, icons::MINI_LOCK, icons::MINI_H, 100, 100, 2);
    c.setTextDatum(textdatum_t::top_left);
    c.setTextSize(1);
    c.setTextColor(GRAY);
    c.drawString(tr(Str::LockHint), 122, 104);
}
