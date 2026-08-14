#include "BriefingApp.h"
#include <ctime>
#include "../core/AppManager.h"
#include "../core/BriefingService.h"
#include "../core/CalendarStore.h"
#include "../core/Lang.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/PoppySprites.h"

BriefingApp briefingApp;

namespace {
constexpr int MAX_ROWS = 6;    // visibles a la vez; con ; . se scrollea
constexpr int MAX_ITEMS = 24;  // tope razonable de cosas en un mismo día

struct DayItem {
    const models::Event* e;
    bool overlaps;
};

// Eventos de hoy ordenados por hora + marca de cruces entre reuniones
int collectToday(DayItem* out, int maxOut, int& conflicts) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    const time_t day0 = mktime(&t);
    const time_t day1 = day0 + 86400;

    const models::Event* evs = calendarStore.events();
    const int n = calendarStore.count();
    int cnt = 0;
    for (int i = 0; i < n && cnt < maxOut; ++i) {
        if (evs[i].start >= day0 && evs[i].start < day1) out[cnt++] = {&evs[i], false};
    }
    for (int i = 1; i < cnt; ++i) {   // inserción por hora
        const DayItem key = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].e->start > key.e->start) { out[j + 1] = out[j]; --j; }
        out[j + 1] = key;
    }
    // Cruce = dos reuniones (no reminders) cuyos rangos se solapan
    conflicts = 0;
    for (int i = 0; i < cnt; ++i) {
        if (out[i].e->flags & models::EVT_REMINDER) continue;
        for (int j = i + 1; j < cnt; ++j) {
            if (out[j].e->flags & models::EVT_REMINDER) continue;
            if (out[j].e->start < out[i].e->end && out[i].e->start < out[j].e->end) {
                if (!out[i].overlaps) ++conflicts;
                if (!out[j].overlaps) ++conflicts;
                out[i].overlaps = out[j].overlaps = true;
            }
        }
    }
    return cnt;
}
}

void BriefingApp::draw(M5Canvas& c) {
    using namespace theme;
    c.drawRoundRect(2, STATUSBAR_H + 2, SCREEN_W - 4, SCREEN_H - STATUSBAR_H - 4, 6, STEM);

    c.setFont(&fonts::Font0);
    c.setTextDatum(textdatum_t::top_left);
    c.setTextSize(2);
    c.setTextColor(PRIMARY);
    c.drawString(tr(Str::BriefTitle), 14, 22);

    DayItem items[MAX_ITEMS];
    int conflicts = 0;
    _count = collectToday(items, MAX_ITEMS, conflicts);

    c.setTextSize(1);
    if (conflicts > 0) {
        char line[40];
        snprintf(line, sizeof(line), tr(Str::BriefConflictFmt), conflicts);
        c.setTextColor(POPPY);
        c.drawString(line, 14, 42);
    }

    if (_count == 0) {
        // Día libre: el conejo aprueba
        pixelart::draw(c, sprites::RABBIT_IDLE, sprites::RABBIT_H, 104, 52, 2);
        c.setTextDatum(textdatum_t::top_center);
        c.setTextColor(STEM);
        c.drawString(tr(Str::BriefEmpty), SCREEN_W / 2, 92);
    }

    const int listY = (conflicts > 0) ? 54 : 44;
    for (int v = 0; v < MAX_ROWS; ++v) {
        const int i = _scroll + v;
        if (i >= _count) break;
        const models::Event& e = *items[i].e;
        const int y = listY + v * 12;
        struct tm ts;
        localtime_r(&e.start, &ts);
        c.fillRect(14, y, 3, 9, CALENDAR_COLORS[e.calendarId % 4]);
        char line[48];
        snprintf(line, sizeof(line), "%02d:%02d %s%.28s", ts.tm_hour, ts.tm_min,
                 (e.flags & models::EVT_REMINDER) ? "!" : "", e.title);
        // Cruces en poppy: lo primero que tienes que resolver hoy
        c.setTextColor(items[i].overlaps ? POPPY
                       : (e.flags & models::EVT_REMINDER) ? GRAY : PRIMARY);
        c.drawString(line, 22, y + 1);
    }

    // Indicadores de que hay más arriba/abajo
    c.setTextColor(GRAY);
    if (_scroll > 0) c.drawString("^", SCREEN_W - 18, listY);
    if (_scroll + MAX_ROWS < _count) {
        char more[12];
        snprintf(more, sizeof(more), "+%d", _count - _scroll - MAX_ROWS);
        c.drawString(more, SCREEN_W - 26, listY + (MAX_ROWS - 1) * 12 + 1);
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(_count > MAX_ROWS ? Str::BriefScrollHint : Str::AlertDismissHint),
                 14, SCREEN_H - 8);
}

void BriefingApp::onKey(const KeyEvent& e) {
    // ; . scrollean; cualquier otra tecla cierra
    if (e.key == Key::Up && _scroll > 0) {
        --_scroll;
        requestRedraw();
        return;
    }
    if (e.key == Key::Down && _scroll + MAX_ROWS < _count) {
        ++_scroll;
        requestRedraw();
        return;
    }
    if (e.key == Key::Up || e.key == Key::Down) return;   // tope: no cerrar
    briefingService.dismiss();
    appManager.goBack();
}
