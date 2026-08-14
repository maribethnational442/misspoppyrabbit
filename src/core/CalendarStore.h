#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../models/Models.h"

class String;

// ============================================================================
// CalendarStore — dueño único de eventos y calendarios (gemelo de TaskStore:
// un solo escritor en el loop principal, la web encola comandos, las lecturas
// desde otra tarea van con mutex, revision() para saber cuándo refrescar).
//
// 4 calendarios fijos (2 trabajos × 2 clientes), renombrables. Persistencia
// en /mspos/agenda.json. Al cargar se descartan eventos terminados hace más
// de 7 días para acotar la RAM.
// ============================================================================

class CalendarStore {
public:
    static constexpr int MAX_EVENTS = 96;
    static constexpr int NUM_CALENDARS = 4;

    void begin();
    void loop();

    // --- Lectura desde el loop principal ---
    const models::Event* events() const       { return _events; }
    int  count() const                        { return _count; }
    const models::Calendar* calendars() const { return _cals; }
    uint32_t revision() const                 { return _rev; }

    // --- Mutaciones desde el loop principal ---
    bool upsertEvent(const models::Event& e);   // id==0 crea; si existe, edita
    bool removeEvent(uint32_t id);
    bool setFlags(uint32_t id, uint8_t mask);   // OR de flags (AlertService)
    void renameCalendar(uint8_t calId, const char* name);

    // --- Para los handlers web (otra tarea) ---
    bool enqueueUpsert(const models::Event& e);
    // Variante con espera acotada: el import mete decenas de eventos y la
    // cola es corta; esperar unos ms deja que el loop principal la drene.
    bool enqueueUpsertWait(const models::Event& e, uint32_t maxWaitMs);
    bool enqueueRemove(uint32_t id);
    bool enqueueRename(uint8_t calId, const char* name);

    void snapshotJson(String& out);   // {"calendars":[...],"events":[...]}

private:
    enum Op : uint8_t { OP_UPSERT, OP_REMOVE, OP_RENAME };
    struct Cmd {
        uint8_t       op;
        uint32_t      id;
        uint8_t       calId;
        char          name[24];
        models::Event ev;
    };

    bool enqueue(const Cmd& cmd);
    void apply(const Cmd& cmd);
    models::Event* findById(uint32_t id);
    void bump();
    void saveNow();
    void seedCalendars();

    models::Event    _events[MAX_EVENTS];
    int              _count = 0;
    models::Calendar _cals[NUM_CALENDARS];
    uint32_t _nextId = 1;
    volatile uint32_t _rev = 1;

    QueueHandle_t     _queue = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    bool     _dirtySave = false;
    uint32_t _saveAt = 0;
};

extern CalendarStore calendarStore;
