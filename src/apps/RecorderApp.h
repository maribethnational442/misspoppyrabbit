#pragma once
#include "../core/App.h"

// Pantalla de grabación de nota de voz. Se abre desde Notes ([m]) o desde
// CUALQUIER pantalla con Fn+M (combo global). Al guardar/cancelar, goBack()
// te devuelve exactamente donde estabas.
class RecorderApp : public App {
public:
    const char* name() const override { return "Rec"; }

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool preventsScreenSleep() const override { return true; }

private:
    bool     _failed = false;
    uint32_t _blinkT = 0;
    bool     _blinkOn = true;
};

extern RecorderApp recorderApp;
