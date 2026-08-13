#pragma once
#include <M5Cardputer.h>

// Tonos del sistema. Speaker.tone() es asíncrono: dispara y sigue, no
// bloquea la UI. Centralizado aquí para que en v0.4 las alertas de la
// Agenda usen la misma "voz" del OS.
namespace sound {

inline void click()      { M5Cardputer.Speaker.tone(2000, 30); }   // tecla/acción
inline void workDone()   { M5Cardputer.Speaker.tone(880, 400); }   // fin de trabajo
inline void breakDone()  { M5Cardputer.Speaker.tone(1319, 400); }  // fin de descanso

} // namespace sound
