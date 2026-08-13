#pragma once
#include "../core/App.h"

// Reloj a pantalla completa. La hora llega por NTP cuando hay WiFi
// (la sincroniza WifiService al conectar).
class ClockApp : public App {
public:
    const char* name() const override { return "Reloj"; }
    const char* const* icon() const override;

    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;

private:
    int _lastSec = -1;
};
