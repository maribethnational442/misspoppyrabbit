#pragma once

// ============================================================================
// Icons.h — iconos 16×16 de las apps, mismo formato de mapa de caracteres
// que los sprites (ver PixelArt.h).
// ============================================================================

namespace icons {

constexpr int ICON_W = 16;
constexpr int ICON_H = 16;

// Engranaje (Settings)
static const char* const GEAR[ICON_H] = {
    "                ",
    "      ####      ",
    "   #  ####  #   ",
    "  ############  ",
    "  ############  ",
    "   ####  ####   ",
    " #####    ##### ",
    " #####    ##### ",
    " #####    ##### ",
    "   ####  ####   ",
    "  ############  ",
    "  ############  ",
    "   #  ####  #   ",
    "      ####      ",
    "                ",
    "                ",
};

// Reloj
static const char* const CLOCK[ICON_H] = {
    "                ",
    "     ######     ",
    "   ##      ##   ",
    "  #          #  ",
    "  #     #    #  ",
    " #      #     # ",
    " #      #     # ",
    " #      ###   # ",
    " #            # ",
    "  #          #  ",
    "  #          #  ",
    "   ##      ##   ",
    "     ######     ",
    "                ",
    "                ",
    "                ",
};

// Información (About)
static const char* const INFO[ICON_H] = {
    "                ",
    "     ######     ",
    "   ##      ##   ",
    "  #    ##    #  ",
    "  #    ##    #  ",
    " #            # ",
    " #     ##     # ",
    " #     ##     # ",
    " #     ##     # ",
    "  #    ##    #  ",
    "  #          #  ",
    "   ##      ##   ",
    "     ######     ",
    "                ",
    "                ",
    "                ",
};

} // namespace icons
