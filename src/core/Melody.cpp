#include "Melody.h"
#include <M5Cardputer.h>

MelodyPlayer melodyPlayer;

void MelodyPlayer::play(const Note* notes, int count, uint32_t repeatGapMs) {
    _notes = notes;
    _count = count;
    _gap = repeatGapMs;
    _idx = 0;
    _nextAt = millis();   // la primera nota suena ya
    _active = true;
}

void MelodyPlayer::stop() {
    _active = false;
    M5Cardputer.Speaker.stop();
}

void MelodyPlayer::loop() {
    if (!_active || millis() < _nextAt) return;

    if (_idx < _count) {
        const Note& n = _notes[_idx++];
        if (n.freq > 0) M5Cardputer.Speaker.tone(n.freq, n.ms);
        _nextAt = millis() + n.ms + 25;   // pequeño aire entre notas
        return;
    }
    // Melodía terminada: ¿repetir o parar?
    if (_gap == 0) {
        _active = false;
        return;
    }
    _idx = 0;
    _nextAt = millis() + _gap;
}
