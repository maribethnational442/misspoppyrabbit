#pragma once
#include <FS.h>
#include <functional>

// ============================================================================
// StorageService — la microSD como servicio del sistema (mismo patrón que
// WifiService). Monta la tarjeta al arrancar y ofrece la primitiva clave:
// escritura ATÓMICA. Escribir "a lo bruto" sobre un archivo significa que si
// la batería se va a mitad de escritura, pierdes tus datos; escribiendo a un
// .tmp y renombrando al final, o queda la versión vieja o la nueva — nunca
// una a medias.
// ============================================================================

class StorageService {
public:
    void begin();
    bool mounted() const { return _mounted; }

    // Escribe con el patrón tmp+rename. `writer` recibe el archivo abierto y
    // devuelve false si algo falló (en ese caso no se toca el original).
    bool atomicWrite(const char* path, std::function<bool(File&)> writer);

private:
    bool _mounted = false;
};

extern StorageService storage;
