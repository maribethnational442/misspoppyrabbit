#pragma once
#include <M5Cardputer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "App.h"
#include "StatusBar.h"

// ============================================================================
// AppManager — el "kernel" de Miss Poppy Rabbit.
//
// * Mantiene una PILA de apps: launch() apila, goBack() desapila. Solo la app
//   del tope recibe update/draw/teclas. La pila modela "volver atrás" gratis:
//   Launcher → Settings → (teclado de texto) → ESC → ESC → Launcher.
// * Mantiene un REGISTRO de apps lanzables (lo que lista el Launcher).
// * Es dueño del ÚNICO canvas a pantalla completa (240×135×16bit ≈ 63KB de
//   los ~300KB de heap). Todas las apps pintan sobre él y se vuelca entero
//   con pushSprite → cero parpadeo. Un solo canvas compartido, y estático,
//   para no fragmentar el heap (sin PSRAM la fragmentación es un enemigo real).
// ============================================================================

class AppManager {
public:
    static constexpr int MAX_STACK = 8;
    static constexpr int MAX_APPS  = 16;

    void begin();
    void registerApp(App* app);      // añade al registro del launcher
    void launch(App* app);           // apila y da el foco
    void goBack();                   // desapila (no-op si solo queda una)
    void tick();                     // UN paso del loop: teclas → update → draw

    // Acceso al registro (lo usa el Launcher para listarse)
    App* const* apps() const  { return _registry; }
    int  appCount() const     { return _count; }

    M5Canvas& canvas()        { return _canvas; }
    App* topApp() const       { return _depth > 0 ? _stack[_depth - 1] : nullptr; }

    // --- Ahorro de energía ---
    uint32_t idleMs() const;      // ms desde la última tecla
    void wakeScreen();            // reenciende si estaba apagada
    bool screenOff() const        { return _screenOff; }

    // --- Combos globales (Fn+L bloqueo, Fn+M grabadora, Fn+N nota) ---
    // main.cpp inyecta las apps: core/ no debe conocer apps concretas.
    void setSystemApps(App* lockApp, App* recorderApp, App* notesApp);
    void lockDevice();
    void unlockDevice();
    bool locked() const           { return _locked; }

    // Inyección de teclas desde otra tarea (POST /api/key): herramienta de
    // depuración y del rig de screenshots. Se drena en tick().
    void injectKey(const KeyEvent& e);

private:
    void pollKeyboard();
    void dispatchKey(const KeyEvent& e);
    void render(bool full);
    void removeFromStack(App* app);

    App*     _stack[MAX_STACK] = {nullptr};
    int      _depth = 0;
    App*     _registry[MAX_APPS] = {nullptr};
    int      _count = 0;

    M5Canvas  _canvas{&M5Cardputer.Display};
    StatusBar _statusBar;
    uint32_t  _lastTick = 0;
    uint32_t  _lastStatusRefresh = 0;
    uint32_t  _lastActivity = 0;
    bool      _screenOff = false;
    bool      _locked = false;
    App*      _lockApp = nullptr;
    App*      _recApp = nullptr;
    App*      _noteApp = nullptr;
    QueueHandle_t _keyQueue = nullptr;
};

// Instancia global única (patrón habitual en firmware: objetos de sistema
// globales y estáticos en vez de singletons dinámicos).
extern AppManager appManager;
