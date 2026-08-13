#pragma once

// ============================================================================
// Config.h — constantes de configuración del sistema (no visuales).
// ============================================================================

namespace config {

constexpr const char* OS_NAME    = "Ms P OS";
constexpr const char* OS_VERSION = "0.1.0";
constexpr const char* HOSTNAME   = "mspos";     // nombre en la red (v0.3: http://mspos.local)

// Zona horaria en formato POSIX TZ. Bogotá = GMT-5 sin horario de verano.
// (Ojo: en POSIX el signo va invertido — "COT5" significa UTC menos 5.)
constexpr const char* TIMEZONE   = "COT5";
constexpr const char* NTP_SERVER = "pool.ntp.org";

// Namespace de NVS (memoria flash de config). Un solo namespace para todo el OS.
constexpr const char* NVS_NS = "mspos";

} // namespace config
