#include "PomodoroService.h"
#include <Arduino.h>
#include "Sound.h"

PomodoroService pomodoroService;

uint32_t PomodoroService::totalMs() const {
    return ((_phase == Phase::Work) ? _workMin : _breakMin) * 60000u;
}

uint32_t PomodoroService::remainingMs() const {
    switch (_run) {
        case Run::Running: {
            const uint32_t now = millis();
            return (now >= _endAt) ? 0 : _endAt - now;
        }
        case Run::Paused:  return _pausedRemain;
        case Run::Ready:   return totalMs();
        default:           return 0;
    }
}

void PomodoroService::loop() {
    // Comando pendiente llegado desde la tarea del servidor web
    if (_webCmd != 0) {
        const uint8_t cmd = _webCmd;
        _webCmd = 0;
        if (cmd == 1) primaryAction();
        else if (cmd == 2) reset();
    }

    // El tramo termina aquí, en el servicio: suena estés donde estés
    if (_run == Run::Running && remainingMs() == 0) finishPhase();
}

void PomodoroService::primaryAction() {
    switch (_run) {
        case Run::Ready:
            startPhase();
            break;
        case Run::Running:
            _pausedRemain = remainingMs();
            _run = Run::Paused;
            sound::click();
            break;
        case Run::Paused:
            startPhase();   // reanuda con lo restante
            break;
        case Run::Finished:
            _phase = (_phase == Phase::Work) ? Phase::Break : Phase::Work;
            _run = Run::Ready;
            startPhase();
            break;
    }
    bump();
}

void PomodoroService::reset() {
    _phase = Phase::Work;
    _run = Run::Ready;
    bump();
}

void PomodoroService::adjustWork(int deltaMin) {
    if (_run != Run::Ready || _phase != Phase::Work) return;
    const int v = _workMin + deltaMin;
    if (v >= 5 && v <= 60) {
        _workMin = v;
        bump();
    }
}

void PomodoroService::startPhase() {
    _endAt = millis() + remainingMs();
    _run = Run::Running;
    sound::click();
}

void PomodoroService::finishPhase() {
    _run = Run::Finished;
    if (_phase == Phase::Work) {
        ++_todayCount;
        sound::workDone();
    } else {
        sound::breakDone();
    }
    bump();
}
