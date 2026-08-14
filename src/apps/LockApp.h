#pragma once
#include "../core/App.h"

// Pantalla de bloqueo (Fn+L). No aparece en el launcher: el AppManager la
// apila al bloquear y la retira al desbloquear. Mientras está bloqueado,
// TODAS las teclas se tragan antes de llegar aquí (salvo Fn+L), así que
// esta app es puramente visual: conejo dormido, la hora, y la pista.
class LockApp : public App {
public:
    const char* name() const override { return "Lock"; }

    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;

private:
    int _lastMin = -1;
};

extern LockApp lockApp;
