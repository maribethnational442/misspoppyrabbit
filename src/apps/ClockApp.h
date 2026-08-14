#pragma once
#include "../core/App.h"
#include "../core/Lang.h"

// Reloj a pantalla completa con los próximos 2 eventos/recordatorios.
// No se apaga por inactividad: a los 2 min pasa a "modo noche" (brillo
// mínimo y esfera simplificada en rojo tenue). Cualquier tecla lo despierta.
class ClockApp : public App {
public:
    const char* name() const override { return tr(Str::AppClock); }
    const char* const* icon() const override;

    void onEnter() override;
    void onExit() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool preventsScreenSleep() const override { return true; }

private:
    void enterNight();
    void exitNight();
    void drawUpcoming(M5Canvas& c);

    int  _lastSec = -1;
    bool _night = false;
};
