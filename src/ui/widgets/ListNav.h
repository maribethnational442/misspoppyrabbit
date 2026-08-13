#pragma once
#include "../../core/App.h"

// Navegación de lista con scroll: selección + ventana visible.
// Solo la matemática; el pintado lo hace cada app (cada lista se ve distinta).
struct ListNav {
    int sel = 0;
    int scroll = 0;
    int visible = 6;   // filas que caben en pantalla

    void reset() { sel = 0; scroll = 0; }

    // Si el nº de elementos bajó (p.ej. se borró una tarea), reencuadra.
    void clampTo(int count) {
        if (sel >= count) sel = (count > 0) ? count - 1 : 0;
        if (scroll > sel) scroll = sel;
    }

    // Devuelve true si la selección cambió (la app debe repintar).
    bool onKey(const KeyEvent& e, int count) {
        if (count <= 0) return false;
        if (e.key == Key::Up && sel > 0) --sel;
        else if (e.key == Key::Down && sel < count - 1) ++sel;
        else return false;
        if (sel < scroll) scroll = sel;                       // asoma por arriba
        if (sel >= scroll + visible) scroll = sel - visible + 1; // por abajo
        return true;
    }
};
