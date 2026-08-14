#pragma once

// ============================================================================
// TimeZones — zona horaria seleccionable en Settings (persistida en NVS).
// Lista curada: el catálogo IANA completo no cabe ni hace falta. El formato
// es POSIX TZ, que es lo que entiende la libc del ESP32 (tzset).
// ============================================================================

namespace tzones {

void begin();               // carga de NVS y aplica (llamar antes de NTP)
void cycle(int dir);        // siguiente/anterior zona; aplica y persiste
const char* label();        // "Bogota", "Madrid"... para el menú
const char* posix();        // cadena POSIX TZ activa

} // namespace tzones
