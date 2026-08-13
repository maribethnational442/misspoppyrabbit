#pragma once

// ============================================================================
// Config.h — constantes de configuración del sistema (no visuales).
// ============================================================================

namespace config {

constexpr const char* OS_NAME    = "Ms P OS";
constexpr const char* OS_VERSION = "0.1.0";
constexpr const char* HOSTNAME   = "mspos";     // nombre en la red (v0.3: http://mspos.local)

// Zona horaria en formato POSIX TZ. AJUSTA ESTA LÍNEA a la tuya:
//   Madrid:  "CET-1CEST,M3.5.0,M10.5.0/3"
//   Bogotá:  "COT5"   |  CDMX: "CST6"  |  Santiago: "<-04>4<-03>,M9.1.6/24,M4.1.6/24"
constexpr const char* TIMEZONE   = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr const char* NTP_SERVER = "pool.ntp.org";

// Namespace de NVS (memoria flash de config). Un solo namespace para todo el OS.
constexpr const char* NVS_NS = "mspos";

} // namespace config
