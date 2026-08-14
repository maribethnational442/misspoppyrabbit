#pragma once
#include <M5GFX.h>

// ============================================================================
// App.h — contrato que cumple toda app de Miss Poppy Rabbit.
//
// El AppManager llama estos métodos en este orden de vida:
//   onEnter() → [ update(dt) → draw(canvas) ]* → onExit()
//
// Regla de oro: update() corre en cada tick (~30/s) pero draw() SOLO cuando
// la app marcó requestRedraw(). Repintar + volcar la pantalla por SPI cuesta
// ~10ms; no hacerlo cuando nada cambió ahorra CPU y batería.
// ============================================================================

// Evento de tecla ya traducido: las apps no saben nada de la matriz física.
// En el Cardputer usamos ; . , / como flechas (arriba/abajo/izq/dcha),
// ENTER como Ok, ` (ESC) como Volver y DEL como borrar.
enum class Key : uint8_t { None, Up, Down, Left, Right, Ok, Back, Backspace, Char };

struct KeyEvent {
    Key  key = Key::None;
    char ch  = 0;   // solo válido cuando key == Key::Char

    // Constructores explícitos: con inicializadores de miembro por defecto,
    // C++11 no permite construir con llaves {Key::Ok, 0} sin esto.
    KeyEvent() = default;
    KeyEvent(Key k, char c = 0) : key(k), ch(c) {}
};

class App {
public:
    virtual ~App() = default;

    virtual const char* name() const = 0;
    // Icono 16×16 en formato mapa de caracteres (ver PixelArt.h); nullptr = sin icono.
    virtual const char* const* icon() const { return nullptr; }

    virtual void onEnter() {}                    // pasa a primer plano
    virtual void onExit() {}                     // sale de la pila
    virtual void update(uint32_t dtMs) { (void)dtMs; }
    virtual void draw(M5Canvas& c) = 0;          // pintar el frame COMPLETO
    virtual void onKey(const KeyEvent& e) { (void)e; }

    // Si devuelve true, el teclado entrega ; . , / como caracteres normales
    // en vez de flechas (necesario para escribir contraseñas). ` sigue
    // siendo Volver y ENTER/DEL siguen siendo Ok/Backspace.
    virtual bool wantsTextInput() const { return false; }

    // --- gestión del flag de repintado (lo usa el AppManager) ---
    void requestRedraw()      { _dirty = true; }
    bool dirty() const        { return _dirty; }
    void clearDirty()         { _dirty = false; }

private:
    bool _dirty = true;   // true al crear: el primer frame siempre se pinta
};
