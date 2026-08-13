#include "AppManager.h"
#include "../ui/Theme.h"

AppManager appManager;

void AppManager::begin() {
    _canvas.setColorDepth(16);
    if (_canvas.createSprite(theme::SCREEN_W, theme::SCREEN_H) == nullptr) {
        // Sin canvas no hay UI: lo dejamos registrado por serial y seguimos
        // (la pantalla mostrará basura, pero el log nos dirá el porqué).
        log_e("No hay RAM para el canvas de %dx%d", theme::SCREEN_W, theme::SCREEN_H);
    }
    _canvas.setTextWrap(false);
}

void AppManager::registerApp(App* app) {
    if (_count < MAX_APPS) _registry[_count++] = app;
}

void AppManager::launch(App* app) {
    if (_depth >= MAX_STACK || app == nullptr) return;
    _stack[_depth++] = app;
    app->onEnter();
    app->requestRedraw();
}

void AppManager::goBack() {
    if (_depth <= 1) return;          // el Launcher nunca se desapila
    _stack[--_depth]->onExit();
    topApp()->onEnter();              // la app revelada retoma el foco
    topApp()->requestRedraw();
}

void AppManager::tick() {
    // Ritmo fijo de ~30fps: si aún no toca, cedemos CPU (delay(1) deja
    // respirar al WiFi y al resto de tareas de FreeRTOS).
    const uint32_t now = millis();
    if (now - _lastTick < theme::FRAME_MS) {
        delay(1);
        return;
    }
    const uint32_t dt = (_lastTick == 0) ? theme::FRAME_MS : (now - _lastTick);
    _lastTick = now;

    M5Cardputer.update();             // refresca teclado, batería, etc.
    pollKeyboard();

    App* top = topApp();
    if (top == nullptr) return;
    top->update(dt);

    // La status bar cambia sola (reloj/batería): forzamos un repintado 1/s
    // aunque la app no haya pedido nada.
    const bool statusDue = (now - _lastStatusRefresh) >= 1000;
    if (top->dirty() || statusDue) {
        render(true);
        top->clearDirty();
        _lastStatusRefresh = now;
    }
}

void AppManager::render(bool full) {
    (void)full;
    _canvas.fillSprite(theme::BG);
    topApp()->draw(_canvas);
    _statusBar.draw(_canvas);         // después de la app: siempre encima
    _canvas.pushSprite(0, 0);         // volcado completo → sin parpadeo
}

// --- Teclado ----------------------------------------------------------------
// Traduce el estado crudo de M5Cardputer.Keyboard a KeyEvent semánticos.
// Convención Cardputer (igual que otros launchers de la comunidad):
//   ';' arriba   '.' abajo   ',' izquierda   '/' derecha
//   ENTER = Ok   '`' (tecla ESC) = Volver   DEL = borrar carácter
void AppManager::pollKeyboard() {
    if (!M5Cardputer.Keyboard.isChange()) return;   // solo en flancos: sin autorepeat
    if (!M5Cardputer.Keyboard.isPressed()) return;  // ignoramos la liberación

    const auto st = M5Cardputer.Keyboard.keysState();

    if (st.enter) { dispatchKey({Key::Ok, 0}); return; }
    if (st.del)   { dispatchKey({Key::Backspace, 0}); return; }

    App* top = topApp();
    const bool textMode = (top != nullptr) && top->wantsTextInput();

    for (auto ch : st.word) {
        KeyEvent e;
        if (textMode && ch != '`') {   // en modo texto solo ` conserva su rol
            e.key = Key::Char;
            e.ch = ch;
            dispatchKey(e);
            continue;
        }
        switch (ch) {
            case ';': e.key = Key::Up;    break;
            case '.': e.key = Key::Down;  break;
            case ',': e.key = Key::Left;  break;
            case '/': e.key = Key::Right; break;
            case '`': e.key = Key::Back;  break;
            default:  e.key = Key::Char; e.ch = ch; break;
        }
        dispatchKey(e);
    }
}

void AppManager::dispatchKey(const KeyEvent& e) {
    App* top = topApp();
    if (top != nullptr) top->onKey(e);
}
