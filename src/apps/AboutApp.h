#pragma once
#include "../core/App.h"

// Ficha del sistema: versión, hardware y RAM libre en vivo (útil para
// vigilar el heap en un equipo sin PSRAM).
class AboutApp : public App {
public:
    const char* name() const override { return "About"; }
    const char* const* icon() const override;

    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;

private:
    uint32_t _accum = 0;
};
