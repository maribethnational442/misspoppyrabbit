#pragma once

// ============================================================================
// PoppySprites.h — la mascota de Miss Poppy Rabbit: conejo blanco con una poppy roja
// en la oreja derecha. 16×16 píxeles, 3 fotogramas (reposo, parpadeo,
// oreja doblada). Edita los strings directamente para iterar el diseño:
// ' '=transparente  '#'=blanco  'R'=poppy  'G'=tallo  '-'=gris oscuro
// ============================================================================

namespace sprites {

constexpr int RABBIT_W = 16;
constexpr int RABBIT_H = 16;

// Fotograma base: ojos abiertos, orejas arriba.
static const char* const RABBIT_IDLE[RABBIT_H] = {
    "   ##    ##     ",
    "  ####  ####    ",
    "  ####  ####    ",
    "  ####  ####    ",
    "  ####  #### RR ",
    "  ##########RRRR",
    " ###########RRRR",
    " ##  ######  RR ",
    " ##  ######  G  ",
    " ############G  ",
    " ######RR#####  ",
    "  #####RR####   ",
    "  ###########   ",
    "   #########    ",
    "    #######     ",
    "                ",
};

// Parpadeo: los ojos (huecos negros) se cierran en una línea.
static const char* const RABBIT_BLINK[RABBIT_H] = {
    "   ##    ##     ",
    "  ####  ####    ",
    "  ####  ####    ",
    "  ####  ####    ",
    "  ####  #### RR ",
    "  ##########RRRR",
    " ###########RRRR",
    " ############RR ",
    " ##--######--G  ",
    " ############G  ",
    " ######RR#####  ",
    "  #####RR####   ",
    "  ###########   ",
    "   #########    ",
    "    #######     ",
    "                ",
};

// Tic de oreja: la oreja izquierda se dobla un instante.
static const char* const RABBIT_TWITCH[RABBIT_H] = {
    "         ##     ",
    "   ##   ####    ",
    "  ####  ####    ",
    "  ####  ####    ",
    "  ####  #### RR ",
    "  ##########RRRR",
    " ###########RRRR",
    " ##  ######  RR ",
    " ##  ######  G  ",
    " ############G  ",
    " ######RR#####  ",
    "  #####RR####   ",
    "  ###########   ",
    "   #########    ",
    "    #######     ",
    "                ",
};

// Reacción del launcher: orejas desplegadas = "¡atención!". Se muestra un
// instante al cambiar de item en el menú.
static const char* const RABBIT_PERK[RABBIT_H] = {
    " ##        ##   ",
    "  ##      ##    ",
    "  ###    ###    ",
    "  ####  ####    ",
    "  ####  #### RR ",
    "  ##########RRRR",
    " ###########RRRR",
    " ##  ######  RR ",
    " ##  ######  G  ",
    " ############G  ",
    " ######RR#####  ",
    "  #####RR####   ",
    "  ###########   ",
    "   #########    ",
    "    #######     ",
    "                ",
};

// --- Props del launcher: objeto 8×8 que el conejo "presenta" según el item
// seleccionado. Mismo formato editable que todo lo demás. RAM: cero (flash).
constexpr int PROP_H = 8;

static const char* const PROP_AGENDA[PROP_H] = {
    "RRRRRRRR",
    "#      #",
    "# # # ##",
    "#      #",
    "# # # ##",
    "#      #",
    "########",
    "        ",
};

static const char* const PROP_TASKS[PROP_H] = {
    "      GG",
    "     GGG",
    "GG  GGG ",
    "GGGGGG  ",
    " GGGG   ",
    "  GG    ",
    "        ",
    "        ",
};

static const char* const PROP_POMODORO[PROP_H] = {
    "   G    ",
    "  GGG   ",
    " RRRRRR ",
    "RRRRRRRR",
    "RRRRRRRR",
    "RRRRRRRR",
    " RRRRRR ",
    "  RRRR  ",
};

static const char* const PROP_SETTINGS[PROP_H] = {
    "   ##   ",
    " # ## # ",
    " ###### ",
    "###  ###",
    "###  ###",
    " ###### ",
    " # ## # ",
    "   ##   ",
};

static const char* const PROP_CLOCK[PROP_H] = {
    "  ####  ",
    " #    # ",
    "#   #  #",
    "#   #  #",
    "#   ## #",
    " #    # ",
    "  ####  ",
    "        ",
};

static const char* const PROP_ABOUT[PROP_H] = {
    " RR RR  ",
    "RRRRRRR ",
    "RRR-RRR ",
    " RRRRR  ",
    "   G    ",
    "   G G  ",
    "   GG   ",
    "        ",
};

} // namespace sprites
