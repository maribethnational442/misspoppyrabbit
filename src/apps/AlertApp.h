#pragma once
#include "../core/App.h"
#include "../ui/widgets/Marquee.h"

// Banner de alerta a pantalla completa. No se abre desde el launcher:
// main.cpp la lanza automáticamente cuando AlertService dispara, encima de
// la app que estuviera abierta (gracias a la pila de apps, al cerrarse
// vuelves exactamente donde estabas).
class AlertApp : public App {
public:
    const char* name() const override { return "Alert"; }

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool preventsScreenSleep() const override { return true; }  // visible siempre

private:
    Marquee _titleScroll;   // los títulos de reuniones suelen ser largos
    Marquee _headScroll;    // "Are you in this meeting?" tampoco cabe a size 2
    uint32_t _blinkT = 0;
    bool _blinkOn = true;
};

extern AlertApp alertApp;
