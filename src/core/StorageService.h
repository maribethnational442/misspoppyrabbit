#pragma once
#include <FS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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
    // Toma el mutex de SD internamente.
    bool atomicWrite(const char* path, std::function<bool(File&)> writer);

    SemaphoreHandle_t sdMutex() const { return _mutex; }

private:
    bool _mounted = false;
    SemaphoreHandle_t _mutex = nullptr;
};

extern StorageService storage;

// RAII del mutex GLOBAL de la SD. Desde v0.7 la tarjeta se toca desde DOS
// tareas (loop principal y servidor web sirviendo notas de voz por chunks)
// y la lib SD NO es thread-safe: TODO acceso a SD debe ir bajo un SdLock.
// No es reentrante: no anidar (atomicWrite ya lo toma por dentro).
class SdLock {
public:
    SdLock() {
        if (storage.sdMutex() != nullptr) xSemaphoreTake(storage.sdMutex(), portMAX_DELAY);
    }
    ~SdLock() {
        if (storage.sdMutex() != nullptr) xSemaphoreGive(storage.sdMutex());
    }
};
