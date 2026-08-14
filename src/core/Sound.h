#pragma once
#include <M5Cardputer.h>

// Tonos del sistema. Speaker.tone() es asíncrono: dispara y sigue, no
// bloquea la UI. Centralizado aquí para que en v0.4 las alertas de la
// Agenda usen la misma "voz" del OS.
namespace sound {

inline void click()      { M5Cardputer.Speaker.tone(2000, 30); }   // tecla/acción
inline void workDone()   { M5Cardputer.Speaker.tone(880, 400); }   // fin de trabajo
inline void breakDone()  { M5Cardputer.Speaker.tone(1319, 400); }  // fin de descanso

// Alertas de agenda, en escalada: cuanto más cerca, más insistente
inline void alert10min() { M5Cardputer.Speaker.tone(988, 250); }   // aviso suave
inline void alert2min()  { M5Cardputer.Speaker.tone(1319, 500); }  // ya casi!
inline void alertNag()   { M5Cardputer.Speaker.tone(1568, 700); }  // ¿estás ahí?

} // namespace sound
