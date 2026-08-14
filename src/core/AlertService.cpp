#include "AlertService.h"
#include <Arduino.h>
#include <ctime>
#include "CalendarStore.h"
#include "Melody.h"

AlertService alertService;

namespace {
// Melodías de alerta: melódicas pero imposibles de ignorar. Se repiten en
// bucle (con su pausa) hasta que el usuario cierra el banner.
// Aviso de 10 min: arpegio ascendente amable (do-mi-sol-do)
const Note MELODY_PRE10[] = {
    {523, 120}, {659, 120}, {784, 120}, {1047, 220},
};
// Aviso de 2 min: dos llamadas urgentes subiendo
const Note MELODY_PRE2[] = {
    {659, 100}, {784, 100}, {988, 140}, {0, 60},
    {659, 100}, {784, 100}, {1175, 260},
};
// "¿Estás en la reunión?": campanilla insistente, fastidiosa con ritmo
const Note MELODY_CONFIRM[] = {
    {1047, 90}, {784, 90}, {1047, 90}, {784, 90},
    {1047, 160}, {0, 70}, {1319, 320},
};
constexpr uint32_t GAP_PRE10   = 6000;   // se repite cada ~6s
constexpr uint32_t GAP_PRE2    = 3500;
constexpr uint32_t GAP_CONFIRM = 2000;   // el más insistente
}

void AlertService::loop() {
    melodyPlayer.loop();                               // la melodía avanza siempre
    if (active()) return;                              // un banner a la vez
    const uint32_t nowMs = millis();
    if (nowMs - _lastCheck < CHECK_EVERY_MS) return;
    _lastCheck = nowMs;

    const time_t now = time(nullptr);
    if (now < 1600000000) return;                      // sin hora real aún (NTP)

    const models::Event* evs = calendarStore.events();
    const int n = calendarStore.count();

    for (int i = 0; i < n; ++i) {
        const models::Event& e = evs[i];
        if (e.flags & models::EVT_DONE) continue;
        if (now >= e.end) continue;                    // ya terminó

        // Los reminders no tienen avisos previos: solo la alarma a su hora
        if (e.flags & models::EVT_REMINDER) {
            if (now >= e.start && !(e.flags & models::EVT_CONFIRMED)) {
                time_t* nextAt = nagSlot(e.id, e.start);
                if (nextAt != nullptr && now >= *nextAt) {
                    *nextAt = now + NAG_EVERY_S;
                    fire(e, Kind::Confirm);
                    return;
                }
            }
            continue;
        }

        // Escalada previa al evento
        if (now < e.start) {
            const time_t toStart = e.start - now;
            if (toStart <= PRE2_S && !(e.flags & models::EVT_ALERT2)) {
                calendarStore.setFlags(e.id, models::EVT_ALERT2 | models::EVT_ALERT10);
                fire(e, Kind::Pre2);
                return;
            }
            if (toStart <= PRE10_S && !(e.flags & models::EVT_ALERT10)) {
                calendarStore.setFlags(e.id, models::EVT_ALERT10);
                fire(e, Kind::Pre10);
                return;
            }
            continue;
        }

        // El evento ya empezó: modo "¿estás ahí?" cada 5 min hasta confirmar
        if (!(e.flags & models::EVT_CONFIRMED)) {
            time_t* nextAt = nagSlot(e.id, e.start);
            if (nextAt != nullptr && now >= *nextAt) {
                *nextAt = now + NAG_EVERY_S;
                fire(e, Kind::Confirm);
                return;
            }
        }
    }
}

void AlertService::fire(const models::Event& e, Kind k) {
    _event = e;   // copia: si la web edita el evento mientras, el banner no muta
    _kind = k;
    // La melodía se repite en bucle hasta que el banner se cierre
    switch (k) {
        case Kind::Pre10:
            melodyPlayer.play(MELODY_PRE10, sizeof(MELODY_PRE10) / sizeof(Note), GAP_PRE10);
            break;
        case Kind::Pre2:
            melodyPlayer.play(MELODY_PRE2, sizeof(MELODY_PRE2) / sizeof(Note), GAP_PRE2);
            break;
        case Kind::Confirm:
            melodyPlayer.play(MELODY_CONFIRM, sizeof(MELODY_CONFIRM) / sizeof(Note), GAP_CONFIRM);
            break;
        default:
            break;
    }
}

void AlertService::dismiss(bool confirmed) {
    melodyPlayer.stop();   // silencio al cerrar el banner
    if (_kind == Kind::Confirm && confirmed) {
        calendarStore.setFlags(_event.id, models::EVT_CONFIRMED);
    }
    _kind = Kind::None;
}

// Busca (o crea) el registro de nag de un evento en curso.
time_t* AlertService::nagSlot(uint32_t id, time_t startAt) {
    const time_t now = time(nullptr);
    int freeIdx = -1;
    for (int i = 0; i < MAX_NAGS; ++i) {
        if (_nags[i].id == id) return &_nags[i].nextAt;
        // Slot libre o de un evento cuyo nag quedó muy atrás (reciclable)
        if (_nags[i].id == 0 || _nags[i].nextAt < now - 24 * 3600) freeIdx = i;
    }
    if (freeIdx < 0) return nullptr;
    _nags[freeIdx].id = id;
    _nags[freeIdx].nextAt = startAt;   // primer "¿estás ahí?" a la hora de inicio
    return &_nags[freeIdx].nextAt;
}
