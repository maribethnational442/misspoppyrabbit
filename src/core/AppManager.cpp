#include "AppManager.h"
#include "../ui/Theme.h"
#include "Config.h"

AppManager appManager;

uint32_t AppManager::idleMs() const {
    return millis() - _lastActivity;
}

void AppManager::wakeScreen() {
    _lastActivity = millis();
    if (!_screenOff) return;
    _screenOff = false;
    M5Cardputer.Display.setBrightness(config::BRIGHTNESS);
    if (topApp() != nullptr) topApp()->requestRedraw();
}

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

// Saca una app de la pila esté donde esté (p.ej. la LockApp cuando una
// alerta se apiló encima del bloqueo).
void AppManager::removeFromStack(App* app) {
    for (int i = _depth - 1; i >= 0; --i) {
        if (_stack[i] != app) continue;
        app->onExit();
        for (int j = i; j < _depth - 1; ++j) _stack[j] = _stack[j + 1];
        --_depth;
        if (i == _depth && topApp() != nullptr) {   // era el tope
            topApp()->onEnter();
        }
        if (topApp() != nullptr) topApp()->requestRedraw();
        return;
    }
}

void AppManager::setSystemApps(App* lockApp, App* recorderApp) {
    _lockApp = lockApp;
    _recApp = recorderApp;
}

void AppManager::lockDevice() {
    if (_locked || _lockApp == nullptr) return;
    _locked = true;
    if (topApp() != _lockApp) launch(_lockApp);
    // Al bolsillo: pantalla fuera inmediatamente
    _screenOff = true;
    M5Cardputer.Display.setBrightness(0);
}

void AppManager::unlockDevice() {
    if (!_locked) return;
    _locked = false;
    wakeScreen();
    removeFromStack(_lockApp);
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

    // Apagado por inactividad (salvo apps que lo veten, como Reloj o Alerta).
    // En una LCD el gasto real es el backlight: apagarlo ES el modo ahorro.
    // Bloqueado, el timeout es corto: la pantalla de bloqueo solo asoma 10s.
    const uint32_t offAfter = _locked ? 10000 : config::SCREEN_OFF_AFTER_MS;
    if (!_screenOff && !top->preventsScreenSleep() &&
        idleMs() > offAfter) {
        _screenOff = true;
        M5Cardputer.Display.setBrightness(0);
    }
    if (_screenOff) return;           // apps en pausa; los servicios siguen

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

    // --- Combos globales Fn+tecla: ANTES que todo lo demás -------------------
    bool fnL = false, fnM = false;
    if (st.fn) {
        for (auto ch : st.word) {
            if (ch == 'l') fnL = true;
            if (ch == 'm') fnM = true;
        }
    }
    if (fnL && _lockApp != nullptr) {   // Fn+L: bloquear/desbloquear
        if (_locked) unlockDevice();
        else lockDevice();
        return;
    }
    if (_locked) {
        // Bloqueado: cualquier tecla solo asoma la pantalla de bloqueo 10s
        wakeScreen();
        return;
    }

    // La tecla que despierta la pantalla se "traga": despertar no debe
    // ejecutar acciones en la app que estaba debajo. (Excepto Fn+M, que
    // despierta Y abre la grabadora: captura rápida hasta con pantalla off.)
    if (_screenOff && !fnM) {
        wakeScreen();
        return;
    }
    _lastActivity = millis();

    if (fnM && _recApp != nullptr) {    // Fn+M: grabadora desde cualquier sitio
        wakeScreen();
        if (topApp() != _recApp) launch(_recApp);
        return;
    }

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
