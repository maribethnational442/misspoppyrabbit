#include "ClockApp.h"
#include "../core/AppManager.h"
#include "../ui/Theme.h"
#include "../ui/assets/Icons.h"
#include "../core/Lang.h"
#include <ctime>

const char* const* ClockApp::icon() const { return icons::CLOCK; }

void ClockApp::update(uint32_t dtMs) {
    (void)dtMs;
    // Solo repintamos cuando cambia el segundo: 1 redibujado/s, no 30.
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    if (t.tm_sec != _lastSec) {
        _lastSec = t.tm_sec;
        requestRedraw();
    }
}

void ClockApp::draw(M5Canvas& c) {
    using namespace theme;
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);

    c.setFont(&fonts::Font0);
    c.setTextDatum(textdatum_t::middle_center);

    if (t.tm_year + 1900 < 2020) {
        // Aún sin sincronizar por NTP
        c.setTextSize(1);
        c.setTextColor(GRAY);
        c.drawString(tr(Str::ClockNoTime1), SCREEN_W / 2, 55);
        c.drawString(tr(Str::ClockNoTime2), SCREEN_W / 2, 70);
        c.drawString(tr(Str::ClockNoTime3), SCREEN_W / 2, 82);
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    c.setTextSize(4);
    c.setTextColor(PRIMARY);
    c.drawString(buf, SCREEN_W / 2, 62);

    char date[32];
    snprintf(date, sizeof(date), "%s %d %s %d",
             lang::days()[t.tm_wday], t.tm_mday, lang::months()[t.tm_mon], t.tm_year + 1900);
    c.setTextSize(1);
    c.setTextColor(POPPY);
    c.drawString(date, SCREEN_W / 2, 95);
}

void ClockApp::onKey(const KeyEvent& e) {
    if (e.key == Key::Back) appManager.goBack();
}
