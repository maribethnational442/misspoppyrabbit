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

// Lista de tareas (checkboxes + líneas; el del medio, hecho)
static const char* const TASKS[ICON_H] = {
    "                ",
    " ####  ######## ",
    " #  #  ######## ",
    " ####           ",
    "                ",
    " ####  ######## ",
    " #GG#  ######## ",
    " ####           ",
    "                ",
    " ####  ######## ",
    " #  #  ######## ",
    " ####           ",
    "                ",
    "                ",
    "                ",
    "                ",
};

// Tomate/poppy (Pomodoro)
static const char* const POMODORO[ICON_H] = {
    "       GG       ",
    "     GGGG       ",
    "      GG        ",
    "    RRRRRR      ",
    "  RRRRRRRRRR    ",
    " RRRRRRRRRRRR   ",
    " RRRRRRRRRRRR   ",
    " RRRRRRRRRRRR   ",
    " RRRRRRRRRRRR   ",
    "  RRRRRRRRRR    ",
    "   RRRRRRRR     ",
    "    RRRRRR      ",
    "                ",
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
