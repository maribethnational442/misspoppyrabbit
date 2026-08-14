#include "AlertService.h"
#include <Arduino.h>
#include <ctime>
#include "CalendarStore.h"
#include "Sound.h"

AlertService alertService;

void AlertService::loop() {
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
    switch (k) {
        case Kind::Pre10:   sound::alert10min(); break;
        case Kind::Pre2:    sound::alert2min(); break;
        case Kind::Confirm: sound::alertNag(); break;
        default: break;
    }
}

void AlertService::dismiss(bool confirmed) {
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
