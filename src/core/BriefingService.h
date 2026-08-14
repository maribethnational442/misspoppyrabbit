#pragma once
#include <cstdint>

// ============================================================================
// BriefingService — el resumen del día. A la hora configurada (defecto 8:00)
// dispara UNA vez al día la pantalla de briefing: eventos de hoy, reminders
// y cruces de horario, para prepararte antes de que empiece el fuego.
//
// "Una vez al día" se persiste en NVS: si a esa hora el dispositivo estaba
// apagado, el briefing salta al encenderlo — nunca se pierde.
// ============================================================================

class BriefingService {
public:
    void begin();    // carga hora configurada y último día mostrado (NVS)
    void loop();

    bool active() const { return _active; }
    void dismiss()      { _active = false; }

    // Hora del briefing en minutos desde medianoche (para Settings)
    int  briefMin() const { return _briefMin; }
    void adjustBriefMin(int deltaMin);   // ±paso, con vuelta; persiste

private:
    static constexpr uint32_t CHECK_EVERY_MS = 10000;

    int      _briefMin = 8 * 60;     // 08:00 por defecto
    uint32_t _lastShownKey = 0;      // año*1000 + día del año
    bool     _active = false;
    uint32_t _lastCheck = 0;
};

extern BriefingService briefingService;
