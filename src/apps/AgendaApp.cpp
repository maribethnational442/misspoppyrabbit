#include "AgendaApp.h"
#include "../core/AppManager.h"
#include "../core/CalendarStore.h"
#include "../ui/Theme.h"
#include "../ui/assets/Icons.h"

AgendaApp agendaApp;

namespace {
constexpr int DAY_ROW_H = 14;
constexpr int DAY_LIST_Y = 34;
constexpr int MAX_DAY_ROWS = 6;

// Vista semana: mapeo de horas 7:00-21:00 al alto disponible
constexpr int WK_TOP = 42;
constexpr int WK_H = 78;
constexpr int WK_HOUR_MIN = 7;
constexpr int WK_HOUR_MAX = 21;

// Índices de los eventos de un día, ordenados por hora de inicio.
int collectDay(time_t day0, int* out, int maxOut) {
    const models::Event* evs = calendarStore.events();
    const int n = calendarStore.count();
    const time_t day1 = day0 + 86400;
    int cnt = 0;
    for (int i = 0; i < n && cnt < maxOut; ++i) {
        if (evs[i].start >= day0 && evs[i].start < day1) out[cnt++] = i;
    }
    // Inserción: pocos elementos, no hace falta más
    for (int i = 1; i < cnt; ++i) {
        const int key = out[i];
        int j = i - 1;
        while (j >= 0 && evs[out[j]].start > evs[key].start) {
            out[j + 1] = out[j];
            --j;
        }
        out[j + 1] = key;
    }
    return cnt;
}
}

const char* const* AgendaApp::icon() const { return icons::AGENDA; }

time_t AgendaApp::startOfDay(time_t t) {
    struct tm tmv;
    localtime_r(&t, &tmv);
    tmv.tm_hour = 0;
    tmv.tm_min = 0;
    tmv.tm_sec = 0;
    return mktime(&tmv);
}

void AgendaApp::onEnter() {
    _mode = Mode::Day;
    _day = startOfDay(time(nullptr));
    _lastRev = calendarStore.revision();
}

void AgendaApp::update(uint32_t dtMs) {
    (void)dtMs;
    // La web puede crear/editar eventos mientras miras: repintar al cambiar
    if (calendarStore.revision() != _lastRev) {
        _lastRev = calendarStore.revision();
        requestRedraw();
    }
}

void AgendaApp::draw(M5Canvas& c) {
    switch (_mode) {
        case Mode::Day:        drawDay(c); break;
        case Mode::Week:       drawWeek(c); break;
        case Mode::QuickTitle: drawDay(c); drawQuickTitle(c); break;
        case Mode::QuickWhen:  drawDay(c); drawQuickWhen(c); break;
    }
}

void AgendaApp::drawDay(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    // Cabecera: fecha del día, en poppy si es hoy
    struct tm t;
    localtime_r(&_day, &t);
    const bool isToday = (_day == startOfDay(time(nullptr)));
    char head[32];
    snprintf(head, sizeof(head), "%s %d %s %d",
             lang::days()[t.tm_wday], t.tm_mday, lang::months()[t.tm_mon], t.tm_year + 1900);
    c.setTextSize(2);
    c.setTextColor(isToday ? POPPY : PRIMARY);
    c.drawString(head, PADDING, 17);

    int idx[MAX_DAY_ROWS + 8];
    const int cnt = collectDay(_day, idx, MAX_DAY_ROWS + 8);
    const models::Event* evs = calendarStore.events();

    if (cnt == 0) {
        c.setTextSize(1);
        c.setTextColor(GRAY);
        c.drawString(tr(Str::AgendaEmptyDay), PADDING, DAY_LIST_Y + 6);

        // Día vacío ≠ agenda vacía: señalar cuándo es lo próximo (evita el
        // clásico "sincronicé y no veo nada" un viernes por la tarde)
        const time_t now = time(nullptr);
        const models::Event* next = nullptr;
        for (int i = 0; i < calendarStore.count(); ++i) {
            const models::Event& e = evs[i];
            if (e.start <= now || (e.flags & models::EVT_DONE)) continue;
            if (next == nullptr || e.start < next->start) next = &e;
        }
        if (next != nullptr) {
            struct tm tn;
            localtime_r(&next->start, &tn);
            char line[48];
            snprintf(line, sizeof(line), tr(Str::AgendaNextFmt),
                     lang::days()[tn.tm_wday], tn.tm_mday, lang::months()[tn.tm_mon],
                     tn.tm_hour, tn.tm_min);
            c.setTextColor(STEM);
            c.drawString(line, PADDING, DAY_LIST_Y + 22);
        }
    }

    c.setTextSize(1);
    for (int r = 0; r < cnt && r < MAX_DAY_ROWS; ++r) {
        const models::Event& e = evs[idx[r]];
        const int y = DAY_LIST_Y + r * DAY_ROW_H;
        struct tm ts, te;
        localtime_r(&e.start, &ts);
        localtime_r(&e.end, &te);

        c.fillRect(PADDING, y, 3, DAY_ROW_H - 3, CALENDAR_COLORS[e.calendarId % 4]);
        char line[64];
        if (e.flags & models::EVT_REMINDER) {
            // Reminder: sin rango horario, con '!' de alarma
            snprintf(line, sizeof(line), "%02d:%02d  !%.28s", ts.tm_hour, ts.tm_min, e.title);
        } else {
            snprintf(line, sizeof(line), "%02d:%02d-%02d:%02d %.26s",
                     ts.tm_hour, ts.tm_min, te.tm_hour, te.tm_min, e.title);
        }
        const bool confirmed = e.flags & models::EVT_CONFIRMED;
        c.setTextColor(confirmed ? STEM
                       : (e.flags & models::EVT_REMINDER) ? POPPY : PRIMARY);
        c.drawString(line, PADDING + 8, y + 2);
    }
    if (cnt > MAX_DAY_ROWS) {
        char more[16];
        snprintf(more, sizeof(more), "+%d...", cnt - MAX_DAY_ROWS);
        c.setTextColor(GRAY);
        c.drawString(more, PADDING + 8, DAY_LIST_Y + MAX_DAY_ROWS * DAY_ROW_H);
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::AgendaHintDay), PADDING, SCREEN_H - 2);
}

void AgendaApp::drawWeek(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    // Lunes de la semana del día seleccionado
    struct tm t;
    localtime_r(&_day, &t);
    const time_t weekStart = _day - ((t.tm_wday + 6) % 7) * (time_t)86400;
    const time_t today0 = startOfDay(time(nullptr));

    const int colW = (SCREEN_W - 8) / 7;
    const models::Event* evs = calendarStore.events();

    for (int d = 0; d < 7; ++d) {
        const time_t day0 = weekStart + d * (time_t)86400;
        const int x = 4 + d * colW;
        struct tm td;
        localtime_r(&day0, &td);

        // Cabecera de la columna: inicial del día + número
        const bool isSel = (day0 == _day);
        const bool isToday = (day0 == today0);
        char head[8];
        snprintf(head, sizeof(head), "%.1s%d", lang::days()[td.tm_wday], td.tm_mday);
        c.setTextColor(isToday ? POPPY : (isSel ? PRIMARY : GRAY));
        c.drawString(head, x + 3, 20);

        if (isSel) c.drawRoundRect(x, 17, colW - 2, SCREEN_H - 17 - 12, 3, POPPY);

        // Bloques de eventos, posicionados por hora (7:00-21:00)
        int idx[12];
        const int cnt = collectDay(day0, idx, 12);
        for (int i = 0; i < cnt; ++i) {
            const models::Event& e = evs[idx[i]];
            struct tm ts;
            localtime_r(&e.start, &ts);
            float h0 = ts.tm_hour + ts.tm_min / 60.0f;
            if (h0 < WK_HOUR_MIN) h0 = WK_HOUR_MIN;
            if (h0 > WK_HOUR_MAX) h0 = WK_HOUR_MAX;
            const float durH = (float)(e.end - e.start) / 3600.0f;
            const int y = WK_TOP + (int)((h0 - WK_HOUR_MIN) * WK_H / (WK_HOUR_MAX - WK_HOUR_MIN));
            int bh = (int)(durH * WK_H / (WK_HOUR_MAX - WK_HOUR_MIN));
            if (bh < 4) bh = 4;
            if (y + bh > WK_TOP + WK_H) bh = WK_TOP + WK_H - y;
            c.fillRoundRect(x + 3, y, colW - 8, bh, 2, CALENDAR_COLORS[e.calendarId % 4]);
        }
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::AgendaHintWeek), PADDING, SCREEN_H - 2);
}

void AgendaApp::drawQuickTitle(M5Canvas& c) {
    using namespace theme;
    c.fillRoundRect(10, 38, SCREEN_W - 20, 60, 5, BG);
    c.drawRoundRect(10, 38, SCREEN_W - 20, 60, 5, POPPY);
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);
    c.setTextColor(PRIMARY);
    c.drawString(tr(Str::QuickTitle), 18, 46);
    _title.draw(c, 18, 58, SCREEN_W - 36);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::QuickSaveHint), 18, 84);
}

void AgendaApp::drawQuickWhen(M5Canvas& c) {
    using namespace theme;
    c.fillRoundRect(10, 30, SCREEN_W - 20, 84, 5, BG);
    c.drawRoundRect(10, 30, SCREEN_W - 20, 84, 5, POPPY);
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    c.setTextColor(PRIMARY);
    c.drawString(_title.buf, 18, 38);

    // Día + hora grandes
    char when[32];
    snprintf(when, sizeof(when), "%s  %02d:%02d",
             tr(_qDayOffset == 0 ? Str::QuickToday : Str::QuickTomorrow),
             _qMinOfDay / 60, _qMinOfDay % 60);
    c.setTextSize(2);
    c.drawString(when, 18, 52);
    if (_qReminder) {   // '!' grande en poppy = esto es un reminder
        c.setTextColor(POPPY);
        c.drawString("!", 18 + c.textWidth(when) + 10, 52);
        c.setTextColor(PRIMARY);
    }

    // Calendario elegido con su color
    c.setTextSize(1);
    c.fillRect(18, 74, 8, 8, CALENDAR_COLORS[_qCal]);
    c.setTextColor(GRAY);
    c.drawString(calendarStore.calendars()[_qCal].name, 30, 74);

    // Pistas en tres líneas cortas: una larga se saldría de los 240px
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::QuickWhenHint), 18, 86);
    c.drawString(tr(Str::QuickRemToggle), 18, 96);
    c.drawString(tr(Str::QuickSaveHint), 18, 106);
}

void AgendaApp::saveQuickEvent() {
    struct tm t;
    const time_t base = startOfDay(time(nullptr)) + _qDayOffset * (time_t)86400;
    localtime_r(&base, &t);
    t.tm_hour = _qMinOfDay / 60;
    t.tm_min = _qMinOfDay % 60;
    t.tm_sec = 0;

    models::Event e = {};
    e.id = 0;   // el store asigna
    strncpy(e.title, _title.buf, sizeof(e.title) - 1);
    e.start = mktime(&t);
    e.end = e.start + 3600;   // reunión de 1h, o ventana de nag del reminder
    e.calendarId = (uint8_t)_qCal;
    e.alertMinBefore = 10;
    e.flags = _qReminder ? models::EVT_REMINDER : 0;
    calendarStore.upsertEvent(e);

    _day = startOfDay(e.start);   // enseña el día donde quedó
    _mode = Mode::Day;
}

void AgendaApp::onKey(const KeyEvent& e) {
    switch (_mode) {
        case Mode::QuickTitle:
            if (_title.handleKey(e)) {
                requestRedraw();
            } else if (e.key == Key::Ok && _title.len > 0) {
                // Hora por defecto: la próxima hora en punto
                struct tm t;
                const time_t now = time(nullptr);
                localtime_r(&now, &t);
                _qMinOfDay = (t.tm_hour + 1) * 60;
                if (_qMinOfDay > 23 * 60) _qMinOfDay = 23 * 60;
                _qDayOffset = 0;
                _mode = Mode::QuickWhen;
                requestRedraw();
            } else if (e.key == Key::Back) {
                _mode = Mode::Day;
                requestRedraw();
            }
            return;

        case Mode::QuickWhen:
            switch (e.key) {
                case Key::Up:    _qMinOfDay = (_qMinOfDay + 15) % (24 * 60); break;
                case Key::Down:  _qMinOfDay = (_qMinOfDay + 24 * 60 - 15) % (24 * 60); break;
                case Key::Left:
                case Key::Right: _qDayOffset = 1 - _qDayOffset; break;
                case Key::Char:
                    if (e.ch == 'c') _qCal = (_qCal + 1) % CalendarStore::NUM_CALENDARS;
                    else if (e.ch == 'r') _qReminder = !_qReminder;
                    break;
                case Key::Ok:   saveQuickEvent(); break;
                case Key::Back: _mode = Mode::Day; break;
                default: return;
            }
            requestRedraw();
            return;

        case Mode::Day:
        case Mode::Week:
            switch (e.key) {
                case Key::Left:  _day -= 86400; requestRedraw(); break;
                case Key::Right: _day += 86400; requestRedraw(); break;
                case Key::Up:
                    if (_mode == Mode::Week) { _day -= 7 * 86400; requestRedraw(); }
                    break;
                case Key::Down:
                    if (_mode == Mode::Week) { _day += 7 * 86400; requestRedraw(); }
                    break;
                case Key::Ok:
                    if (_mode == Mode::Week) { _mode = Mode::Day; requestRedraw(); }
                    break;
                case Key::Char:
                    if (e.ch == 't') {
                        _day = startOfDay(time(nullptr));
                    } else if (e.ch == 'v') {
                        _mode = (_mode == Mode::Day) ? Mode::Week : Mode::Day;
                    } else if (e.ch == 'n') {
                        _title.clear();
                        _title.maxLen = (int)sizeof(models::Event{}.title) - 1;
                        _qReminder = false;
                        _mode = Mode::QuickTitle;
                    } else {
                        return;
                    }
                    requestRedraw();
                    break;
                case Key::Back:
                    appManager.goBack();
                    break;
                default:
                    break;
            }
            return;
    }
}
