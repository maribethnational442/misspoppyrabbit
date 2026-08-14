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
constexpr int MAX_ROWS = 6;

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

    DayItem items[MAX_ROWS + 6];
    int conflicts = 0;
    const int cnt = collectToday(items, MAX_ROWS + 6, conflicts);

    c.setTextSize(1);
    if (conflicts > 0) {
        char line[40];
        snprintf(line, sizeof(line), tr(Str::BriefConflictFmt), conflicts);
        c.setTextColor(POPPY);
        c.drawString(line, 14, 42);
    }

    if (cnt == 0) {
        // Día libre: el conejo aprueba
        pixelart::draw(c, sprites::RABBIT_IDLE, sprites::RABBIT_H, 104, 52, 2);
        c.setTextDatum(textdatum_t::top_center);
        c.setTextColor(STEM);
        c.drawString(tr(Str::BriefEmpty), SCREEN_W / 2, 92);
    }

    const int listY = (conflicts > 0) ? 54 : 44;
    for (int i = 0; i < cnt && i < MAX_ROWS; ++i) {
        const models::Event& e = *items[i].e;
        const int y = listY + i * 12;
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
    if (cnt > MAX_ROWS) {
        char more[12];
        snprintf(more, sizeof(more), "+%d...", cnt - MAX_ROWS);
        c.setTextColor(GRAY);
        c.drawString(more, 22, listY + MAX_ROWS * 12 + 1);
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::AlertDismissHint), 14, SCREEN_H - 8);
}

void BriefingApp::onKey(const KeyEvent& e) {
    (void)e;
    briefingService.dismiss();
    appManager.goBack();
}
