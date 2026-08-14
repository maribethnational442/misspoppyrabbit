#pragma once
#include <cstdint>

// ============================================================================
// Config.h — constantes de configuración del sistema (no visuales).
// ============================================================================

namespace config {

constexpr const char* OS_NAME       = "Miss Poppy Rabbit";
constexpr const char* OS_NAME_SHORT = "Miss P. Rabbit";   // para espacios estrechos (launcher)
constexpr const char* OS_VERSION    = "0.7.1";

// --- Pantalla y ahorro de energía ---
// La pantalla es IPS/LCD: lo que gasta es la RETROILUMINACIÓN, no los píxeles.
constexpr int      BRIGHTNESS          = 80;      // brillo normal (0-255)
constexpr int      DIM_BRIGHTNESS      = 8;       // modo noche del Reloj
constexpr uint32_t SCREEN_OFF_AFTER_MS = 150000;  // 2.5 min sin teclas → apagar
constexpr uint32_t CLOCK_DIM_AFTER_MS  = 120000;  // 2 min en el Reloj → modo noche
// Nombre en la red: la WebUI vive en http://prabbit.local
constexpr const char* HOSTNAME   = "prabbit";

// microSD: pines del bus SPI dedicado del Cardputer
constexpr int SD_SCK  = 40;
constexpr int SD_MISO = 39;
constexpr int SD_MOSI = 14;
constexpr int SD_CS   = 12;

// Rutas de datos. Todo lo nuestro vive bajo /mspos para convivir con
// cualquier otra cosa que haya en la tarjeta.
constexpr const char* DATA_DIR    = "/mspos";
constexpr const char* TASKS_FILE  = "/mspos/tasks.json";
constexpr const char* AGENDA_FILE = "/mspos/agenda.json";
constexpr const char* NOTES_DIR   = "/mspos/notes";

// La zona horaria ya no vive aquí: se elige en Settings (core/TimeZones.h)
constexpr const char* NTP_SERVER = "pool.ntp.org";

// Namespace de NVS (memoria flash de config). Un solo namespace para todo el OS.
constexpr const char* NVS_NS = "mspos";

} // namespace config
