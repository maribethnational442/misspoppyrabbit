#include "ClockApp.h"
#include <M5Cardputer.h>
#include <ctime>
#include "../core/AppManager.h"
#include "../core/CalendarStore.h"
#include "../core/Config.h"
#include "../ui/Theme.h"
#include "../ui/assets/Icons.h"

const char* const* ClockApp::icon() const { return icons::CLOCK; }

void ClockApp::onEnter() {
    _night = false;
    _lastSec = -1;
}

void ClockApp::onExit() {
    if (_night) exitNight();
}

void ClockApp::enterNight() {
    _night = true;
    M5Cardputer.Display.setBrightness(config::DIM_BRIGHTNESS);
    requestRedraw();
}

void ClockApp::exitNight() {
    _night = false;
    M5Cardputer.Display.setBrightness(config::BRIGHTNESS);
    requestRedraw();
}

void ClockApp::update(uint32_t dtMs) {
    (void)dtMs;
    // 2 min sin teclas → modo noche (la pantalla general no se apaga aquí:
    // preventsScreenSleep() lo impide; este es nuestro ahorro propio)
    if (!_night && appManager.idleMs() > config::CLOCK_DIM_AFTER_MS) {
        enterNight();
        return;
    }

    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    // De noche basta repintar al cambiar el minuto (no hay segundero)
    const int tick = _night ? t.tm_min : t.tm_sec;
    if (tick != _lastSec) {
        _lastSec = tick;
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
        c.setTextSize(1);
        c.setTextColor(GRAY);
        c.drawString(tr(Str::ClockNoTime1), SCREEN_W / 2, 55);
        c.drawString(tr(Str::ClockNoTime2), SCREEN_W / 2, 70);
        c.drawString(tr(Str::ClockNoTime3), SCREEN_W / 2, 82);
        return;
    }

    if (_night) {
        // Esfera nocturna: solo HH:MM en rojo tenue (clásico reloj de mesilla)
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        c.setTextSize(4);
        c.setTextColor(POPPY_DIM);
        c.drawString(buf, SCREEN_W / 2, SCREEN_H / 2 + 4);
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    c.setTextSize(4);
    // A la 1:17 (y 13:17) la hora florece en poppy. Quien lo note, lo notará.
    const bool poppyMinute = (t.tm_hour % 12 == 1) && (t.tm_min == 17);
    c.setTextColor(poppyMinute ? POPPY : PRIMARY);
    c.drawString(buf, SCREEN_W / 2, 48);

    char date[32];
    snprintf(date, sizeof(date), "%s %d %s %d",
             lang::days()[t.tm_wday], t.tm_mday, lang::months()[t.tm_mon], t.tm_year + 1900);
    c.setTextSize(1);
    c.setTextColor(POPPY);
    c.drawString(date, SCREEN_W / 2, 76);

    drawUpcoming(c);
}

// Los 2 próximos eventos/recordatorios, para que un vistazo al reloj también
// diga "qué viene ahora".
void ClockApp::drawUpcoming(M5Canvas& c) {
    using namespace theme;
    const time_t now = time(nullptr);
    const models::Event* evs = calendarStore.events();
    const int n = calendarStore.count();

    // Los 2 con inicio más cercano en el futuro
    const models::Event* next[2] = {nullptr, nullptr};
    for (int i = 0; i < n; ++i) {
        if (evs[i].start < now || (evs[i].flags & models::EVT_DONE)) continue;
        if (next[0] == nullptr || evs[i].start < next[0]->start) {
            next[1] = next[0];
            next[0] = &evs[i];
        } else if (next[1] == nullptr || evs[i].start < next[1]->start) {
            next[1] = &evs[i];
        }
    }

    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);
    struct tm tn;
    localtime_r(&now, &tn);
    for (int i = 0; i < 2; ++i) {
        if (next[i] == nullptr) break;
        const models::Event& e = *next[i];
        const int y = 92 + i * 14;
        struct tm ts;
        localtime_r(&e.start, &ts);

        c.fillRect(30, y, 3, 10, CALENDAR_COLORS[e.calendarId % 4]);
        char line[48];
        // Si es hoy solo la hora; si no, día + hora. '!' marca los reminders.
        if (ts.tm_yday == tn.tm_yday && ts.tm_year == tn.tm_year) {
            snprintf(line, sizeof(line), "%02d:%02d %s%.24s", ts.tm_hour, ts.tm_min,
                     (e.flags & models::EVT_REMINDER) ? "!" : "", e.title);
        } else {
            snprintf(line, sizeof(line), "%s %02d:%02d %s%.20s",
                     lang::days()[ts.tm_wday], ts.tm_hour, ts.tm_min,
                     (e.flags & models::EVT_REMINDER) ? "!" : "", e.title);
        }
        c.setTextColor((e.flags & models::EVT_REMINDER) ? POPPY : GRAY);
        c.drawString(line, 38, y + 1);
    }
}

void ClockApp::onKey(const KeyEvent& e) {
    if (_night) {
        exitNight();   // cualquier tecla solo despierta, no actúa
        return;
    }
    if (e.key == Key::Back) appManager.goBack();
}
