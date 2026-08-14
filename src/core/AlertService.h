#pragma once
#include <cstdint>
#include "../models/Models.h"

// ============================================================================
// AlertService — alertas de agenda en modo TDAH. Corre siempre (como el
// pomodoro), estés en la app que estés. Escalada por evento:
//
//   T-10 min  → beep suave + banner (se cierra con cualquier tecla)
//   T-2 min   → beep fuerte + banner
//   Al empezar → "¿Ya estás en la reunión?" — ENTER confirma y se calla;
//                si no, vuelve a preguntar cada 5 min hasta confirmar o
//                hasta que el evento termine.
//
// El servicio solo decide CUÁNDO alertar y guarda los flags en el
// CalendarStore; main.cpp lanza AlertApp cuando active() es true.
// ============================================================================

class AlertService {
public:
    enum class Kind : uint8_t { None, Pre10, Pre2, Confirm };

    void loop();

    bool active() const        { return _kind != Kind::None; }
    Kind kind() const          { return _kind; }
    // Copia del evento en el momento de disparar (estable para pintar)
    const models::Event& event() const { return _event; }

    // Llamado por AlertApp al cerrar el banner. En Kind::Confirm,
    // confirmed=true silencia el evento para siempre; false = nag en 5 min.
    void dismiss(bool confirmed);

private:
    void fire(const models::Event& e, Kind k);

    static constexpr uint32_t CHECK_EVERY_MS = 5000;
    static constexpr time_t   PRE10_S = 10 * 60;
    static constexpr time_t   PRE2_S  = 2 * 60;
    static constexpr time_t   NAG_EVERY_S = 5 * 60;

    Kind          _kind = Kind::None;
    models::Event _event = {};
    uint32_t      _lastCheck = 0;

    // Próximo "¿estás ahí?" por evento en curso (pocos a la vez)
    struct Nag { uint32_t id; time_t nextAt; };
    static constexpr int MAX_NAGS = 8;
    Nag _nags[MAX_NAGS] = {};

    time_t* nagSlot(uint32_t id, time_t startAt);
};

extern AlertService alertService;
