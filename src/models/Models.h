#pragma once
#include <ctime>
#include <cstdint>

// ============================================================================
// Models.h — estructuras de datos del roadmap completo, definidas desde v0.1
// para que todo lo que construyamos encima ya hable este esquema.
//
// Diseño para 512KB de SRAM sin PSRAM:
// * Campos de tamaño FIJO (char[N], no String): structs compactos, sin heap,
//   serializables a JSON en microSD por streaming con ArduinoJson.
// * Fechas en time_t (epoch UTC): es lo que enviará la extensión de Chrome
//   en v0.5 y lo que entiende la lib de tiempo del ESP32.
// ============================================================================

namespace models {

// Un "calendario" es un contexto de trabajo (2 trabajos × 2 clientes = 4).
struct Calendar {
    uint8_t  id;
    char     name[24];
    uint16_t color;        // RGB565, se elige de la paleta del tema
};

// Flags de bit para Event/Task (un byte guarda 8 booleanos).
// Las alertas son escalonadas (modo TDAH): 10 min antes, 2 min antes, y al
// empezar pregunta si ya estás en la reunión — insiste cada 5 min hasta
// que confirmes (EVT_CONFIRMED) o el evento termine.
enum EventFlags : uint8_t {
    EVT_DONE      = 1 << 0,   // completado / ya pasó
    EVT_SYNCED    = 1 << 1,   // vino del import de v0.5 (idempotencia)
    EVT_ALERT10   = 1 << 2,   // el aviso de 10 min ya sonó
    EVT_ALERT2    = 1 << 3,   // el aviso de 2 min ya sonó
    EVT_CONFIRMED = 1 << 4,   // el usuario confirmó que está en el evento
    // Reminder = evento puntual sin duración ("Llamar al banco"): sin avisos
    // previos; alarma A LA HORA que insiste cada 5 min hasta marcar "hecho"
    // (reutiliza EVT_CONFIRMED). Su end es start+1h: la ventana del nag.
    EVT_REMINDER  = 1 << 5,
};

// Evento de agenda (v0.4) e importable desde el navegador (v0.5).
struct Event {
    uint32_t id;             // estable entre imports: re-importar no duplica
    char     title[48];
    time_t   start;          // epoch UTC
    time_t   end;
    uint8_t  calendarId;     // → Calendar.color y contexto
    uint8_t  alertMinBefore; // minutos de antelación de la alerta sonora
    uint8_t  flags;          // EventFlags
};

// Tarea (v0.2). Comparte la idea de contexto con los eventos.
struct Task {
    uint32_t id;
    char     title[48];
    time_t   due;            // 0 = sin fecha límite
    uint8_t  calendarId;
    uint8_t  priority;       // 0=baja 1=media 2=alta
    uint8_t  flags;          // EVT_DONE aplica también aquí
};

} // namespace models
