#include "RecorderApp.h"
#include "../core/AppManager.h"
#include "../core/Lang.h"
#include "../core/NotesService.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/Icons.h"

RecorderApp recorderApp;

void RecorderApp::onEnter() {
    _failed = !notesService.recStart();
    _blinkT = 0;
    _blinkOn = true;
}

void RecorderApp::update(uint32_t dtMs) {
    if (_failed) return;
    // Si el límite de tiempo detuvo la grabación (ya guardada), volvemos solos
    if (!notesService.recording()) {
        appManager.goBack();
        return;
    }
    _blinkT += dtMs;
    if (_blinkT >= 400) {
        _blinkT = 0;
        _blinkOn = !_blinkOn;
    }
    requestRedraw();   // contador y VU en vivo (modo exclusivo: nos lo permitimos)
}

void RecorderApp::draw(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);

    if (_failed) {
        c.setTextDatum(textdatum_t::middle_center);
        c.setTextSize(1);
        c.setTextColor(POPPY);
        c.drawString(tr(Str::RecFail), SCREEN_W / 2, SCREEN_H / 2);
        return;
    }

    // Punto REC parpadeante + rótulo
    if (_blinkOn) c.fillCircle(38, 48, 9, POPPY);
    c.drawCircle(38, 48, 9, POPPY_DIM);
    c.setTextDatum(textdatum_t::middle_left);
    c.setTextSize(2);
    c.setTextColor(PRIMARY);
    c.drawString("REC", 56, 48);

    // Tiempo transcurrido / límite
    // Formato compacto: "00:09/1:00" cabe en pantalla (el largo se salía)
    const uint32_t s = notesService.recMs() / 1000;
    char buf[24];
    snprintf(buf, sizeof(buf), "%02u:%02u/%d:%02d",
             (unsigned)(s / 60), (unsigned)(s % 60),
             notesService.recLimitS() / 60, notesService.recLimitS() % 60);
    c.setTextSize(2);
    c.setTextColor(GRAY);
    c.drawString(buf, 108, 48);

    // Barra VU: el nivel del chunk recién capturado
    const int w = (notesService.recLevel() * (SCREEN_W - 56)) / 100;
    c.drawRect(28, 74, SCREEN_W - 56, 12, DARKGRAY);
    c.fillRect(30, 76, w, 8, notesService.recLevel() > 80 ? POPPY : STEM);

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextSize(1);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::RecHint), PADDING, SCREEN_H - 4);
}

void RecorderApp::onKey(const KeyEvent& e) {
    if (_failed) {
        appManager.goBack();
        return;
    }
    if (e.key == Key::Ok) {
        notesService.recStop(true);
        appManager.goBack();
    } else if (e.key == Key::Back) {
        notesService.recStop(false);
        appManager.goBack();
    }
}
