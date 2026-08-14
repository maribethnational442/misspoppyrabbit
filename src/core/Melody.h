#pragma once
#include <cstdint>

// ============================================================================
// MelodyPlayer — melodías de tonos SIN bloquear y SIN samples en RAM.
// Una melodía es un array de notas (frecuencia + duración; freq 0 = silencio).
// loop() avanza nota a nota con millis(); play() puede repetir en bucle con
// una pausa entre repeticiones hasta que alguien llame a stop().
// ============================================================================

struct Note {
    uint16_t freq;   // Hz; 0 = silencio
    uint16_t ms;
};

class MelodyPlayer {
public:
    // repeatGapMs: pausa entre repeticiones; 0 = sonar una sola vez
    void play(const Note* notes, int count, uint32_t repeatGapMs);
    void stop();
    void loop();     // llamar en cada vuelta del loop principal
    bool playing() const { return _active; }

private:
    const Note* _notes = nullptr;
    int      _count = 0;
    int      _idx = 0;
    uint32_t _nextAt = 0;
    uint32_t _gap = 0;
    bool     _active = false;
};

extern MelodyPlayer melodyPlayer;
