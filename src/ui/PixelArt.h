#pragma once
#include <M5GFX.h>
#include "Theme.h"

// ============================================================================
// PixelArt.h — dibuja sprites definidos como "mapas de caracteres":
// un array de strings donde cada carácter es un píxel. Formato elegido a
// propósito para poder editar los sprites a mano en el propio código.
//
//   ' ' transparente   '#' blanco    'R' rojo poppy   'r' poppy oscuro
//   'G' verde tallo    '.' gris      '-' gris oscuro
// ============================================================================

namespace pixelart {

inline uint16_t colorFor(char c) {
    switch (c) {
        case '#': return theme::PRIMARY;
        case 'R': return theme::POPPY;
        case 'r': return theme::POPPY_DIM;
        case 'G': return theme::STEM;
        case '.': return theme::GRAY;
        case '-': return theme::DARKGRAY;
        default:  return theme::BG;
    }
}

// Dibuja el mapa en (x,y). `scale` multiplica el tamaño de cada píxel
// (scale=3 → cada carácter se pinta como un cuadrado de 3×3).
inline void draw(M5Canvas& c, const char* const* rows, int numRows,
                 int x, int y, int scale = 1) {
    for (int ry = 0; ry < numRows; ++ry) {
        for (int rx = 0; rows[ry][rx] != '\0'; ++rx) {
            const char px = rows[ry][rx];
            if (px == ' ') continue;
            if (scale == 1) {
                c.drawPixel(x + rx, y + ry, colorFor(px));
            } else {
                c.fillRect(x + rx * scale, y + ry * scale, scale, scale, colorFor(px));
            }
        }
    }
}

} // namespace pixelart
