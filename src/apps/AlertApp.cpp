#include "AlertApp.h"
#include <ctime>
#include "../core/AppManager.h"
#include "../core/AlertService.h"
#include "../core/CalendarStore.h"
#include "../core/Lang.h"
#include "../ui/Theme.h"

AlertApp alertApp;

void AlertApp::onEnter() {
    _titleScroll.reset();
    _headScroll.reset();
    _blinkT = 0;
    _blinkOn = true;
}

void AlertApp::update(uint32_t dtMs) {
    if (_titleScroll.tick(dtMs)) requestRedraw();
    if (_headScroll.tick(dtMs)) requestRedraw();
    // Borde parpadeante en el modo "¿estás ahí?": urgencia visible
    if (alertService.kind() == AlertService::Kind::Confirm) {
        _blinkT += dtMs;
        if (_blinkT >= 500) {
            _blinkT = 0;
            _blinkOn = !_blinkOn;
            requestRedraw();
        }
    }
}

void AlertApp::draw(M5Canvas& c) {
    using namespace theme;
    const models::Event& e = alertService.event();
    const AlertService::Kind kind = alertService.kind();
    const uint16_t calCol = CALENDAR_COLORS[e.calendarId % 4];

    // Marco: fijo en los avisos previos, parpadeante en el "¿estás ahí?"
    const uint16_t frame = (kind == AlertService::Kind::Confirm && !_blinkOn) ? DARKGRAY : POPPY;
    c.drawRoundRect(2, STATUSBAR_H + 2, SCREEN_W - 4, SCREEN_H - STATUSBAR_H - 4, 6, frame);
    c.drawRoundRect(3, STATUSBAR_H + 3, SCREEN_W - 6, SCREEN_H - STATUSBAR_H - 6, 6, frame);

    c.setFont(&fonts::Font0);
    c.setTextDatum(textdatum_t::top_left);

    // Qué está pasando
    c.setTextSize(2);
    c.setTextColor(kind == AlertService::Kind::Confirm ? POPPY : PRIMARY);
    const Str head = (kind == AlertService::Kind::Pre10) ? Str::AlertIn10
                   : (kind == AlertService::Kind::Pre2)  ? Str::AlertIn2
                                                         : Str::AlertConfirm;
    _headScroll.draw(c, tr(head), 14, 28, SCREEN_W - 28, 18);

    // Título del evento (marquee si no cabe) + franja del calendario
    c.fillRect(14, 52, 4, 24, calCol);
    c.setTextSize(2);
    c.setTextColor(PRIMARY);
    _titleScroll.draw(c, e.title, 24, 56, SCREEN_W - 40, 18);

    // Hora y calendario
    struct tm t;
    localtime_r(&e.start, &t);
    char line[48];
    snprintf(line, sizeof(line), "%02d:%02d  %s",
             t.tm_hour, t.tm_min, calendarStore.calendars()[e.calendarId % 4].name);
    c.setTextSize(1);
    c.setTextColor(GRAY);
    c.drawString(line, 24, 82);

    // Cómo salir
    c.setTextColor(DARKGRAY);
    c.setTextDatum(textdatum_t::bottom_left);
    const Str hint = (kind == AlertService::Kind::Confirm) ? Str::AlertConfirmHint
                                                           : Str::AlertDismissHint;
    c.drawString(tr(hint), 14, SCREEN_H - 8);
}

void AlertApp::onKey(const KeyEvent& e) {
    if (alertService.kind() == AlertService::Kind::Confirm) {
        // Solo ENTER confirma; cualquier otra tecla = "pregúntame en 5 min"
        alertService.dismiss(e.key == Key::Ok);
    } else {
        alertService.dismiss(false);
    }
    appManager.goBack();
}
