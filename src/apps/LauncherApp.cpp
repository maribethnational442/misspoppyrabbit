#include "LauncherApp.h"
#include "../core/AppManager.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/PoppySprites.h"
#include "../core/Config.h"
#include "../core/Lang.h"
#include <cmath>

namespace {
constexpr int LIST_X   = 4;
constexpr int LIST_Y   = 16;    // debajo de la status bar
constexpr int ROW_H    = 18;    // 6 apps caben: 16 + 6*18 = 124 < 135
constexpr int LIST_W   = 148;
constexpr int MASCOT_X = 176;
constexpr int MASCOT_Y = 34;
constexpr int MASCOT_SCALE = 3; // 16×16 → 48×48 px

int cursorTargetY(int sel) { return LIST_Y + sel * ROW_H; }

// Prop que el conejo "presenta" por item. MISMO ORDEN que el registro de
// apps en main.cpp (Agenda, Tareas, Pomodoro, Settings, Reloj, About).
const char* const* const PROPS[] = {
    sprites::PROP_AGENDA, sprites::PROP_TASKS, sprites::PROP_POMODORO,
    sprites::PROP_SETTINGS, sprites::PROP_CLOCK, sprites::PROP_ABOUT,
};
constexpr int NUM_PROPS = sizeof(PROPS) / sizeof(PROPS[0]);
constexpr uint32_t REACT_MS = 400;   // duración de la reacción completa
}

void LauncherApp::onEnter() {
    // Primer enter: el cursor nace ya en su sitio, sin animación de entrada.
    if (_cursorY < 0) _cursorY = cursorTargetY(_sel);
    _reactT = 0;   // saluda presentando el prop del item actual
}

void LauncherApp::update(uint32_t dtMs) {
    // --- Cursor deslizante: interpolación exponencial hacia el objetivo.
    // Cada tick recorre una fracción (dt/100ms) de la distancia restante:
    // rápido al inicio, suave al llegar. Redibujamos solo mientras se mueve.
    const float target = cursorTargetY(_sel);
    const float diff = target - _cursorY;
    if (std::fabs(diff) > 0.5f) {
        float step = diff * (float)dtMs / (float)theme::CURSOR_ANIM_MS;
        // paso mínimo para que siempre llegue aunque diff sea pequeño
        if (std::fabs(step) < 1.0f) step = (diff > 0) ? 1.0f : -1.0f;
        _cursorY += step;
        if ((diff > 0 && _cursorY > target) || (diff < 0 && _cursorY < target))
            _cursorY = target;
        requestRedraw();
    }

    // --- Reacción al cambiar de item: mientras dura, repintamos cada frame
    // (el prop rebota y el conejo despliega las orejas)
    if (_reactT < REACT_MS) {
        _reactT += dtMs;
        requestRedraw();
    }

    // --- Mascota: ciclo de 6s → reposo, parpadeo en 3.0-3.15s, tic de oreja
    // en 5.0-5.4s. Solo repintamos cuando cambia el fotograma.
    _mascotT = (_mascotT + dtMs) % 6000;
    int frame = 0;
    if (_mascotT >= 3000 && _mascotT < 3150) frame = 1;
    else if (_mascotT >= 5000 && _mascotT < 5400) frame = 2;
    if (frame != _mascotFrame) {
        _mascotFrame = frame;
        requestRedraw();
    }
}

void LauncherApp::draw(M5Canvas& c) {
    using namespace theme;

    // Cursor: rectángulo redondeado poppy detrás del ítem seleccionado
    c.fillRoundRect(LIST_X, (int)_cursorY, LIST_W, ROW_H - 2, 4, POPPY_DIM);
    c.drawRoundRect(LIST_X, (int)_cursorY, LIST_W, ROW_H - 2, 4, POPPY);

    // Lista de apps registradas
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::middle_left);
    for (int i = 0; i < appManager.appCount(); ++i) {
        App* app = appManager.apps()[i];
        const int rowY = LIST_Y + i * ROW_H;
        if (app->icon() != nullptr) {
            pixelart::draw(c, app->icon(), 16, LIST_X + 6, rowY, 1);
        }
        c.setTextColor(i == _sel ? PRIMARY : GRAY);
        c.drawString(app->name(), LIST_X + 30, rowY + (ROW_H - 2) / 2);
    }

    // Mascota + nombre del OS a la derecha. Durante la reacción, orejas
    // desplegadas; el resto del tiempo, el ciclo normal de reposo.
    const char* const* mascot =
        (_reactT < 200)       ? sprites::RABBIT_PERK :
        (_mascotFrame == 1)   ? sprites::RABBIT_BLINK :
        (_mascotFrame == 2)   ? sprites::RABBIT_TWITCH : sprites::RABBIT_IDLE;
    pixelart::draw(c, mascot, sprites::RABBIT_H, MASCOT_X, MASCOT_Y, MASCOT_SCALE);

    // Prop del item seleccionado, con botecito al aparecer (cae de -6px a 0)
    if (_sel < NUM_PROPS) {
        const int bounce = (_reactT < 300) ? -(int)((300 - _reactT) / 50) : 0;
        pixelart::draw(c, PROPS[_sel], sprites::PROP_H,
                       MASCOT_X - 22, MASCOT_Y + 14 + bounce, 2);
    }

    c.setTextDatum(textdatum_t::top_center);
    c.setTextColor(POPPY);
    // Centrado bajo la mascota, pero sin salirse del borde derecho
    int nameCx = MASCOT_X + (sprites::RABBIT_W * MASCOT_SCALE) / 2;
    const int half = c.textWidth(config::OS_NAME_SHORT) / 2;
    if (nameCx + half > SCREEN_W - 2) nameCx = SCREEN_W - 2 - half;
    c.drawString(config::OS_NAME_SHORT, nameCx,
                 MASCOT_Y + sprites::RABBIT_H * MASCOT_SCALE + 6);

    // Ayuda de teclas abajo
    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::LauncherHint), LIST_X, SCREEN_H - 2);
}

void LauncherApp::onKey(const KeyEvent& e) {
    const int n = appManager.appCount();
    if (n == 0) return;
    switch (e.key) {
        case Key::Up:
            _sel = (_sel + n - 1) % n;   // con vuelta arriba↔abajo
            _reactT = 0;                 // el conejo reacciona al item nuevo
            requestRedraw();
            break;
        case Key::Down:
            _sel = (_sel + 1) % n;
            _reactT = 0;
            requestRedraw();
            break;
        case Key::Ok:
            appManager.launch(appManager.apps()[_sel]);
            break;
        default:
            break;
    }
}
