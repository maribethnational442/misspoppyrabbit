#pragma once

// ============================================================================
// Config.h — constantes de configuración del sistema (no visuales).
// ============================================================================

namespace config {

constexpr const char* OS_NAME       = "Miss Poppy Rabbit";
constexpr const char* OS_NAME_SHORT = "Miss Poppy";   // para espacios estrechos (launcher)
constexpr const char* OS_VERSION    = "0.3.0";
// El hostname se queda corto a propósito: http://mspos.local es cómodo de
// teclear. Cámbialo aquí si prefieres otro (p.ej. "poppy" → http://poppy.local).
constexpr const char* HOSTNAME   = "mspos";

// microSD: pines del bus SPI dedicado del Cardputer
constexpr int SD_SCK  = 40;
constexpr int SD_MISO = 39;
constexpr int SD_MOSI = 14;
constexpr int SD_CS   = 12;

// Rutas de datos. Todo lo nuestro vive bajo /mspos para convivir con
// cualquier otra cosa que haya en la tarjeta.
constexpr const char* DATA_DIR   = "/mspos";
constexpr const char* TASKS_FILE = "/mspos/tasks.json";

// Zona horaria en formato POSIX TZ. Bogotá = GMT-5 sin horario de verano.
// (Ojo: en POSIX el signo va invertido — "COT5" significa UTC menos 5.)
constexpr const char* TIMEZONE   = "COT5";
constexpr const char* NTP_SERVER = "pool.ntp.org";

// Namespace de NVS (memoria flash de config). Un solo namespace para todo el OS.
constexpr const char* NVS_NS = "mspos";

} // namespace config
