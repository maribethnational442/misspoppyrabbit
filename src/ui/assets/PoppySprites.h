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

} // namespace sprites
