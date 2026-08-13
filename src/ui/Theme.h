#pragma once
#include <cstdint>

// ============================================================================
// Theme.h — ÚNICO punto de verdad de la identidad visual de Ms P OS.
// Cambia aquí un color y cambia en todo el firmware.
// ============================================================================

// La pantalla (ST7789) trabaja en RGB565: 16 bits por píxel (5 de rojo,
// 6 de verde, 5 de azul). Esta función convierte un color RGB de 8 bits por
// canal (como los #RRGGBB de la web) a ese formato, en tiempo de compilación
// (constexpr = se calcula al compilar, no gasta CPU en el dispositivo).
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

namespace theme {

// --- Paleta (conejo blanco + poppy roja sobre negro) -----------------------
constexpr uint16_t BG        = rgb565(0x00, 0x00, 0x00); // fondo negro
constexpr uint16_t PRIMARY   = rgb565(0xFF, 0xFF, 0xFF); // blanco conejo: texto/UI
constexpr uint16_t POPPY     = rgb565(0xE6, 0x39, 0x46); // rojo poppy: cursor/alertas
constexpr uint16_t POPPY_DIM = rgb565(0x8A, 0x22, 0x2A); // poppy oscuro: sombras/acentos
constexpr uint16_t STEM      = rgb565(0x6A, 0x99, 0x4E); // verde tallo: estados OK
constexpr uint16_t GRAY      = rgb565(0x88, 0x88, 0x88); // texto secundario
constexpr uint16_t DARKGRAY  = rgb565(0x2A, 0x2A, 0x2A); // separadores, fondos sutiles

// --- Métricas de layout ----------------------------------------------------
constexpr int SCREEN_W    = 240;
constexpr int SCREEN_H    = 135;
constexpr int STATUSBAR_H = 14;   // barra superior siempre visible
constexpr int PADDING     = 4;

// --- Tiempos de animación --------------------------------------------------
constexpr uint32_t FRAME_MS      = 33;   // ~30 fps de tick del sistema
constexpr uint32_t CURSOR_ANIM_MS = 100; // deslizamiento del cursor del menú

} // namespace theme
