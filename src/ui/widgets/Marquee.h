#pragma once
#include <M5GFX.h>

// Texto que se desliza horizontalmente cuando no cabe en su ancho (marquee).
// Si cabe, se pinta normal y no consume nada.
//
// Uso: la app llama tick(dt) en update() — si devuelve true, requestRedraw()
// — y draw() al pintar. El ciclo: pausa breve → scroll → vuelta → pausa...
struct Marquee {
    float    offset = 0;      // desplazamiento actual en px
    bool     active = false;  // el último draw() detectó que no cabía
    int      textW = 0;
    uint32_t holdMs = 0;      // acumulador de la pausa inicial

    static constexpr int      GAP_PX      = 24;   // hueco entre el final y la vuelta
    static constexpr uint32_t HOLD_MS     = 900;  // pausa antes de scrollear
    static constexpr float    SPEED_PX_MS = 0.03f; // ~30 px/segundo

    void reset() {
        offset = 0;
        holdMs = 0;
    }

    // Devuelve true si hay que repintar este frame.
    bool tick(uint32_t dtMs) {
        if (!active) return false;
        if (holdMs < HOLD_MS) {          // pausa para poder leer el inicio
            holdMs += dtMs;
            return false;
        }
        offset += dtMs * SPEED_PX_MS;
        if (offset >= (float)(textW + GAP_PX)) {   // vuelta completa
            offset = 0;
            holdMs = 0;
        }
        return true;
    }

    // Pinta el texto en (x,y) limitado a `w` px de ancho (fuente/color ya
    // configurados por la app). `h` es el alto de la zona de recorte.
    void draw(M5Canvas& c, const char* text, int x, int y, int w, int h = 10) {
        textW = c.textWidth(text);
        active = textW > w;
        if (!active) {
            c.drawString(text, x, y);
            return;
        }
        // Recortamos al área visible y pintamos dos copias: la que sale por
        // la izquierda y la que entra por la derecha (efecto de vuelta).
        c.setClipRect(x, y, w, h);
        const int base = x - (int)offset;
        c.drawString(text, base, y);
        c.drawString(text, base + textW + GAP_PX, y);
        c.clearClipRect();
    }
};
