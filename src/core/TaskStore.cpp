#include "TaskStore.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstring>
#include "../models/TaskRepo.h"

TaskStore taskStore;

namespace {
constexpr uint32_t SAVE_DEBOUNCE_MS = 1500;

// Guardia RAII del mutex: se toma al construir, se suelta al salir del scope.
// Así es imposible olvidar soltarlo en un return temprano.
struct Lock {
    SemaphoreHandle_t m;
    explicit Lock(SemaphoreHandle_t mtx) : m(mtx) { xSemaphoreTake(m, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(m); }
};
}

void TaskStore::begin() {
    _mutex = xSemaphoreCreateMutex();
    _queue = xQueueCreate(8, sizeof(Cmd));

    const int n = taskrepo::load(_tasks, MAX_TASKS);
    _corrupt = (n == -2);
    _count = (n > 0) ? n : 0;
    for (int i = 0; i < _count; ++i) {
        if (_tasks[i].id >= _nextId) _nextId = _tasks[i].id + 1;
    }
}

void TaskStore::loop() {
    // Consumir lo que la web haya encolado desde la otra tarea
    Cmd cmd;
    while (_queue != nullptr && xQueueReceive(_queue, &cmd, 0) == pdTRUE) {
        apply(cmd);
    }
    if (_dirtySave && millis() >= _saveAt) saveNow();
}

// --- Mutaciones (siempre ejecutadas en el loop principal) -------------------

bool TaskStore::add(const char* title) {
    if (_count >= MAX_TASKS || title == nullptr || title[0] == '\0') return false;
    {
        Lock lock(_mutex);
        models::Task& t = _tasks[_count++];
        t.id = _nextId++;
        strncpy(t.title, title, sizeof(t.title) - 1);
        t.title[sizeof(t.title) - 1] = '\0';
        t.due = 0;
        t.calendarId = 0;
        t.priority = 1;
        t.flags = 0;
    }
    bump();
    return true;
}

bool TaskStore::toggleDone(uint32_t id) {
    Lock lock(_mutex);
    models::Task* t = findById(id);
    if (t == nullptr) return false;
    t->flags ^= models::EVT_DONE;
    bump();
    return true;
}

bool TaskStore::remove(uint32_t id) {
    Lock lock(_mutex);
    for (int i = 0; i < _count; ++i) {
        if (_tasks[i].id != id) continue;
        for (int j = i; j < _count - 1; ++j) _tasks[j] = _tasks[j + 1];
        --_count;
        bump();
        return true;
    }
    return false;
}

bool TaskStore::cyclePriority(uint32_t id) {
    Lock lock(_mutex);
    models::Task* t = findById(id);
    if (t == nullptr) return false;
    t->priority = (t->priority + 1) % 3;
    bump();
    return true;
}

// --- Cola desde la web ------------------------------------------------------

bool TaskStore::enqueue(const Cmd& cmd) {
    // Sin espera: si la cola está llena, el navegador recibe el error y
    // reintenta — jamás bloqueamos la tarea del servidor web.
    return _queue != nullptr && xQueueSend(_queue, &cmd, 0) == pdTRUE;
}

bool TaskStore::enqueueAdd(const char* title) {
    Cmd cmd = {};
    cmd.op = OP_ADD;
    strncpy(cmd.title, title, sizeof(cmd.title) - 1);
    return enqueue(cmd);
}
bool TaskStore::enqueueToggle(uint32_t id)   { Cmd c = {}; c.op = OP_TOGGLE;   c.id = id; return enqueue(c); }
bool TaskStore::enqueueRemove(uint32_t id)   { Cmd c = {}; c.op = OP_REMOVE;   c.id = id; return enqueue(c); }
bool TaskStore::enqueuePriority(uint32_t id) { Cmd c = {}; c.op = OP_PRIORITY; c.id = id; return enqueue(c); }

void TaskStore::apply(const Cmd& cmd) {
    switch (cmd.op) {
        case OP_ADD:      add(cmd.title); break;
        case OP_TOGGLE:   toggleDone(cmd.id); break;
        case OP_REMOVE:   remove(cmd.id); break;
        case OP_PRIORITY: cyclePriority(cmd.id); break;
    }
}

// --- Lectura para la web ----------------------------------------------------

void TaskStore::snapshotJson(String& out) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    {
        Lock lock(_mutex);
        for (int i = 0; i < _count; ++i) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]    = _tasks[i].id;
            o["title"] = _tasks[i].title;
            o["prio"]  = _tasks[i].priority;
            o["done"]  = (bool)(_tasks[i].flags & models::EVT_DONE);
        }
    }
    serializeJson(doc, out);
}

// --- Internos ---------------------------------------------------------------

models::Task* TaskStore::findById(uint32_t id) {
    for (int i = 0; i < _count; ++i) {
        if (_tasks[i].id == id) return &_tasks[i];
    }
    return nullptr;
}

void TaskStore::bump() {
    _rev = _rev + 1;
    _dirtySave = true;
    _saveAt = millis() + SAVE_DEBOUNCE_MS;
}

void TaskStore::saveNow() {
    _dirtySave = false;
    if (!taskrepo::save(_tasks, _count)) {
        log_w("No se pudo guardar tasks.json");
    }
}
