#pragma once
#include "../core/App.h"

// Pantalla del resumen del día. La lanza main.cpp cuando BriefingService
// dispara (no aparece en el launcher). Muestra los eventos de hoy en orden,
// los reminders con '!', y en poppy los que se CRUZAN entre sí — lo que
// necesitas saber antes de que arranque el día.
class BriefingApp : public App {
public:
    const char* name() const override { return "Briefing"; }

    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool preventsScreenSleep() const override { return true; }
};

extern BriefingApp briefingApp;
