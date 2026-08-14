#pragma once
#include "../core/App.h"
#include "../core/Lang.h"

// Cara en pantalla del PomodoroService: pinta el anillo y traduce teclas a
// comandos. Toda la lógica del temporizador vive en el servicio, que corre
// siempre — salir de esta app no detiene nada y el beep suena igual.
class PomodoroApp : public App {
public:
    const char* name() const override { return tr(Str::AppPomodoro); }
    const char* const* icon() const override;

    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;

private:
    uint32_t _lastRev = 0;
    int _lastShownSec = -1;
};
