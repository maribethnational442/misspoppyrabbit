#pragma once
#include "../core/App.h"

// Pomodoro clásico: trabajo (rojo poppy) / descanso (verde tallo), anillo de
// progreso gigante y beep al terminar cada tramo.
//
// El estado vive en el objeto (estático), así que salir al launcher NO pierde
// el temporizador: al volver, el tiempo se recalcula desde _endAt. Limitación
// v0.2: si termina mientras estás en otra app, el beep suena al volver a
// entrar (las alertas en segundo plano llegan como servicio en v0.4).
class PomodoroApp : public App {
public:
    const char* name() const override { return "Pomodoro"; }
    const char* const* icon() const override;

    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;

private:
    enum class Phase : uint8_t { Work, Break };
    enum class Run   : uint8_t { Ready, Running, Paused, Finished };

    void startPhase();
    void finishPhase();
    uint32_t phaseTotalMs() const;
    uint32_t remainingMs() const;

    Phase _phase = Phase::Work;
    Run   _run = Run::Ready;
    int   _workMin = 25;
    int   _breakMin = 5;
    uint32_t _endAt = 0;       // millis() en que termina el tramo actual
    uint32_t _pausedRemain = 0;
    int   _todayCount = 0;     // pomodoros completados en esta sesión
    int   _lastShownSec = -1;
};
