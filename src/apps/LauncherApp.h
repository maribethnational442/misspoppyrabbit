#pragma once
#include "../core/App.h"

// Pantalla principal: lista de apps a la izquierda (cursor deslizante),
// mascota idle animada a la derecha, estilo Flipper Zero.
class LauncherApp : public App {
public:
    const char* name() const override { return "Launcher"; }

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;

private:
    int      _sel = 0;          // app seleccionada
    float    _cursorY = -1;     // posición actual del cursor (animada)
    uint32_t _mascotT = 0;      // reloj de la animación de la mascota
    int      _mascotFrame = 0;
    uint32_t _reactT = 9999;    // ms desde el último cambio de selección
};
