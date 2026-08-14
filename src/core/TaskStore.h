#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../models/Models.h"

class String;

// ============================================================================
// TaskStore — dueño ÚNICO de la lista de tareas. La pantalla (TasksApp) y el
// navegador (WebService) son dos clientes del mismo servicio.
//
// El problema nuevo de v0.3 es la CONCURRENCIA: los handlers del servidor web
// corren en otra tarea de FreeRTOS (otro "hilo"). Si el navegador modificara
// la lista justo mientras la pantalla la recorre, se corrompería. Diseño:
//
//   * ESCRIBE solo el loop principal. La web no toca la lista: encola
//     comandos (enqueue*) en una cola FreeRTOS (thread-safe) que el loop
//     principal consume. Un solo escritor = sin carreras entre escritores.
//   * La web LEE vía snapshotJson(), protegido con un mutex que las
//     mutaciones también toman: nunca se lee a medio modificar.
//   * revision() se incrementa con cada cambio: la UI y el WebSocket saben
//     cuándo refrescar sin comparar listas enteras.
// ============================================================================

class TaskStore {
public:
    static constexpr int MAX_TASKS = 64;

    void begin();   // carga de microSD
    void loop();    // consume comandos encolados + guardado debounced

    // --- Lectura desde el loop principal (la UI): sin lock, mismo hilo ---
    const models::Task* tasks() const { return _tasks; }
    int  count() const     { return _count; }
    bool corrupt() const   { return _corrupt; }
    uint32_t revision() const { return _rev; }

    // --- Mutaciones desde el loop principal (la UI) ---
    void clearAll();   // borra TODAS las tareas (opción de Settings)
    bool add(const char* title);
    bool toggleDone(uint32_t id);
    bool remove(uint32_t id);
    bool cyclePriority(uint32_t id);

    // --- Para los handlers web (OTRA tarea): encolar, nunca tocar ---
    bool enqueueAdd(const char* title);
    bool enqueueToggle(uint32_t id);
    bool enqueueRemove(uint32_t id);
    bool enqueuePriority(uint32_t id);

    // Serializa la lista a JSON. Seguro desde cualquier tarea (toma el mutex).
    void snapshotJson(String& out);

private:
    enum Op : uint8_t { OP_ADD, OP_TOGGLE, OP_REMOVE, OP_PRIORITY };
    struct Cmd {
        uint8_t  op;
        uint32_t id;
        char     title[48];
    };

    bool enqueue(const Cmd& cmd);
    void apply(const Cmd& cmd);
    models::Task* findById(uint32_t id);
    void bump();      // ++revision + programa guardado
    void saveNow();

    models::Task _tasks[MAX_TASKS];
    int  _count = 0;
    bool _corrupt = false;
    uint32_t _nextId = 1;
    volatile uint32_t _rev = 1;

    QueueHandle_t     _queue = nullptr;
    SemaphoreHandle_t _mutex = nullptr;

    bool     _dirtySave = false;
    uint32_t _saveAt = 0;
};

extern TaskStore taskStore;
