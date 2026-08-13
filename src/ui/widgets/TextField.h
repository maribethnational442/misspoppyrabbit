#pragma once
#include <M5GFX.h>
#include "../../core/App.h"
#include "../Theme.h"

// Campo de texto de una línea con cursor. La app dueña debe activar
// wantsTextInput() mientras el campo tenga el foco, y decidir qué hacer
// con Ok (confirmar) y Back (cancelar) — el widget solo edita el buffer.
struct TextField {
    char buf[65] = {0};
    int  len = 0;
    int  maxLen = 64;   // bajar para campos más cortos (p.ej. 47 en tareas)

    void clear() {
        len = 0;
        buf[0] = '\0';
    }

    // Devuelve true si el texto cambió (la app debe repintar).
    bool handleKey(const KeyEvent& e) {
        if (e.key == Key::Char && e.ch >= 32 && len < maxLen) {
            buf[len++] = e.ch;
            buf[len] = '\0';
            return true;
        }
        if (e.key == Key::Backspace && len > 0) {
            buf[--len] = '\0';
            return true;
        }
        return false;
    }

    void draw(M5Canvas& c, int x, int y, int w) const {
        using namespace theme;
        c.drawRoundRect(x, y, w, 18, 3, POPPY);
        c.setFont(&fonts::Font0);
        c.setTextSize(1);
        c.setTextDatum(textdatum_t::top_left);
        c.setTextColor(PRIMARY);
        // Si el texto es más ancho que la caja, muestra la cola (donde escribes)
        const char* shown = buf;
        while (*shown && c.textWidth(shown) > w - 12) ++shown;
        c.drawString(shown, x + 5, y + 5);
        c.drawFastVLine(x + 6 + c.textWidth(shown), y + 3, 12, POPPY);
    }
};
