#include "LauncherApp.h"
#include "../core/AppManager.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/PoppySprites.h"
#include "../core/Config.h"
#include <cmath>

namespace {
constexpr int LIST_X   = 4;
constexpr int LIST_Y   = 18;    // debajo de la status bar
constexpr int ROW_H    = 20;    // 5 apps caben: 18 + 5*20 = 118 < 135
constexpr int LIST_W   = 148;
constexpr int MASCOT_X = 176;
constexpr int MASCOT_Y = 34;
constexpr int MASCOT_SCALE = 3; // 16×16 → 48×48 px

int cursorTargetY(int sel) { return LIST_Y + sel * ROW_H; }
}

void LauncherApp::onEnter() {
    // Primer enter: el cursor nace ya en su sitio, sin animación de entrada.
    if (_cursorY < 0) _cursorY = cursorTargetY(_sel);
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
            pixelart::draw(c, app->icon(), 16, LIST_X + 6, rowY + 1, 1);
        }
        c.setTextColor(i == _sel ? PRIMARY : GRAY);
        c.drawString(app->name(), LIST_X + 30, rowY + (ROW_H - 2) / 2);
    }

    // Mascota + nombre del OS a la derecha
    const char* const* mascot =
        (_mascotFrame == 1) ? sprites::RABBIT_BLINK :
        (_mascotFrame == 2) ? sprites::RABBIT_TWITCH : sprites::RABBIT_IDLE;
    pixelart::draw(c, mascot, sprites::RABBIT_H, MASCOT_X, MASCOT_Y, MASCOT_SCALE);

    c.setTextDatum(textdatum_t::top_center);
    c.setTextColor(POPPY);
    c.drawString(config::OS_NAME_SHORT,   // el nombre completo no cabe aqui
                 MASCOT_X + (sprites::RABBIT_W * MASCOT_SCALE) / 2,
                 MASCOT_Y + sprites::RABBIT_H * MASCOT_SCALE + 6);

    // Ayuda de teclas abajo
    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString("[;] [.] mover   [ENTER] abrir", LIST_X, SCREEN_H - 2);
}

void LauncherApp::onKey(const KeyEvent& e) {
    const int n = appManager.appCount();
    if (n == 0) return;
    switch (e.key) {
        case Key::Up:
            _sel = (_sel + n - 1) % n;   // con vuelta arriba↔abajo
            requestRedraw();
            break;
        case Key::Down:
            _sel = (_sel + 1) % n;
            requestRedraw();
            break;
        case Key::Ok:
            appManager.launch(appManager.apps()[_sel]);
            break;
        default:
            break;
    }
}
