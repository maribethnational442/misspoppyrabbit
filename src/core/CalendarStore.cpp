#include "CalendarStore.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <cstring>
#include <ctime>
#include "Config.h"
#include "StorageService.h"
#include "../ui/Theme.h"

CalendarStore calendarStore;

namespace {
constexpr uint32_t SAVE_DEBOUNCE_MS = 1500;
constexpr time_t   PRUNE_AGE_S = 7 * 24 * 3600;   // eventos de hace >7 días fuera

struct Lock {
    SemaphoreHandle_t m;
    explicit Lock(SemaphoreHandle_t mtx) : m(mtx) { xSemaphoreTake(m, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(m); }
};

void copyName(char* dst, size_t cap, const char* src) {
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}
}

void CalendarStore::seedCalendars() {
    static const char* const DEFAULTS[NUM_CALENDARS] = {
        "Trabajo 1", "T1 Cliente A", "Trabajo 2", "T2 Cliente A",
    };
    for (int i = 0; i < NUM_CALENDARS; ++i) {
        _cals[i].id = (uint8_t)i;
        copyName(_cals[i].name, sizeof(_cals[i].name), DEFAULTS[i]);
        _cals[i].color = theme::CALENDAR_COLORS[i];
    }
}

void CalendarStore::begin() {
    _mutex = xSemaphoreCreateMutex();
    _queue = xQueueCreate(16, sizeof(Cmd));
    seedCalendars();

    if (!storage.mounted()) return;
    File f = SD.open(config::AGENDA_FILE, FILE_READ);
    if (!f) return;

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        log_e("agenda.json corrupto: %s", err.c_str());
        return;
    }

    for (JsonObject o : doc["calendars"].as<JsonArray>()) {
        const uint8_t id = o["id"] | (uint8_t)0;
        if (id < NUM_CALENDARS) copyName(_cals[id].name, sizeof(_cals[id].name), o["name"] | "");
    }

    // Poda: solo si el RTC ya tiene hora real (al arrancar aún puede ser 1970)
    const time_t now = time(nullptr);
    const bool clockOk = (now > 1600000000);   // ~2020

    for (JsonObject o : doc["events"].as<JsonArray>()) {
        if (_count >= MAX_EVENTS) break;
        models::Event e = {};
        e.id = o["id"] | 0u;
        copyName(e.title, sizeof(e.title), o["title"] | "");
        e.start          = (time_t)(o["start"] | 0u);
        e.end            = (time_t)(o["end"] | 0u);
        e.calendarId     = o["cal"] | (uint8_t)0;
        e.alertMinBefore = o["alert"] | (uint8_t)10;
        e.flags          = o["flags"] | (uint8_t)0;
        if (clockOk && e.end < now - PRUNE_AGE_S) continue;   // demasiado viejo
        _events[_count++] = e;
        if (e.id >= _nextId) _nextId = e.id + 1;
    }
}

void CalendarStore::loop() {
    Cmd cmd;
    while (_queue != nullptr && xQueueReceive(_queue, &cmd, 0) == pdTRUE) {
        apply(cmd);
    }
    if (_dirtySave && millis() >= _saveAt) saveNow();
}

// --- Mutaciones (solo loop principal) ---------------------------------------

bool CalendarStore::upsertEvent(const models::Event& e) {
    Lock lock(_mutex);
    if (e.id != 0) {
        models::Event* dst = findById(e.id);
        if (dst != nullptr) {
            // Idempotencia: si nada cambió (re-import del mismo calendario),
            // no tocamos nada — ni revision ni escritura a SD.
            if (dst->start == e.start && dst->end == e.end &&
                dst->calendarId == e.calendarId &&
                strcmp(dst->title, e.title) == 0) {
                return true;
            }
            models::Event upd = e;
            if (dst->start == e.start && dst->end == e.end) {
                // Mismo horario: conservar el estado de alertas ya disparadas
                upd.flags |= dst->flags & (models::EVT_ALERT10 | models::EVT_ALERT2 |
                                           models::EVT_CONFIRMED);
            } else {
                // Movieron la reunión: las alertas se rearman para la hora nueva
                upd.flags &= ~(models::EVT_ALERT10 | models::EVT_ALERT2 |
                               models::EVT_CONFIRMED);
            }
            *dst = upd;
            bump();
            return true;
        }
    }
    if (_count >= MAX_EVENTS || e.title[0] == '\0') return false;
    _events[_count] = e;
    _events[_count].id = (e.id != 0) ? e.id : _nextId++;
    if (_events[_count].id >= _nextId) _nextId = _events[_count].id + 1;
    ++_count;
    bump();
    return true;
}

bool CalendarStore::removeEvent(uint32_t id) {
    Lock lock(_mutex);
    for (int i = 0; i < _count; ++i) {
        if (_events[i].id != id) continue;
        for (int j = i; j < _count - 1; ++j) _events[j] = _events[j + 1];
        --_count;
        bump();
        return true;
    }
    return false;
}

bool CalendarStore::setFlags(uint32_t id, uint8_t mask) {
    Lock lock(_mutex);
    models::Event* e = findById(id);
    if (e == nullptr) return false;
    e->flags |= mask;
    bump();
    return true;
}

void CalendarStore::renameCalendar(uint8_t calId, const char* name) {
    if (calId >= NUM_CALENDARS || name == nullptr || name[0] == '\0') return;
    Lock lock(_mutex);
    copyName(_cals[calId].name, sizeof(_cals[calId].name), name);
    bump();
}

// --- Cola desde la web ------------------------------------------------------

bool CalendarStore::enqueue(const Cmd& cmd) {
    return _queue != nullptr && xQueueSend(_queue, &cmd, 0) == pdTRUE;
}

bool CalendarStore::enqueueUpsert(const models::Event& e) {
    Cmd c = {};
    c.op = OP_UPSERT;
    c.ev = e;
    return enqueue(c);
}

bool CalendarStore::enqueueUpsertWait(const models::Event& e, uint32_t maxWaitMs) {
    Cmd c = {};
    c.op = OP_UPSERT;
    c.ev = e;
    return _queue != nullptr &&
           xQueueSend(_queue, &c, pdMS_TO_TICKS(maxWaitMs)) == pdTRUE;
}
bool CalendarStore::enqueueRemove(uint32_t id) {
    Cmd c = {};
    c.op = OP_REMOVE;
    c.id = id;
    return enqueue(c);
}
bool CalendarStore::enqueueRename(uint8_t calId, const char* name) {
    Cmd c = {};
    c.op = OP_RENAME;
    c.calId = calId;
    copyName(c.name, sizeof(c.name), name);
    return enqueue(c);
}

void CalendarStore::apply(const Cmd& cmd) {
    switch (cmd.op) {
        case OP_UPSERT: upsertEvent(cmd.ev); break;
        case OP_REMOVE: removeEvent(cmd.id); break;
        case OP_RENAME: renameCalendar(cmd.calId, cmd.name); break;
    }
}

// --- Snapshot para la web ---------------------------------------------------

void CalendarStore::snapshotJson(String& out) {
    JsonDocument doc;
    JsonArray cals = doc["calendars"].to<JsonArray>();
    JsonArray evs  = doc["events"].to<JsonArray>();
    {
        Lock lock(_mutex);
        for (int i = 0; i < NUM_CALENDARS; ++i) {
            JsonObject o = cals.add<JsonObject>();
            o["id"]   = _cals[i].id;
            o["name"] = _cals[i].name;
        }
        for (int i = 0; i < _count; ++i) {
            JsonObject o = evs.add<JsonObject>();
            o["id"]    = _events[i].id;
            o["title"] = _events[i].title;
            o["start"] = (uint32_t)_events[i].start;
            o["end"]   = (uint32_t)_events[i].end;
            o["cal"]   = _events[i].calendarId;
            o["done"]  = (bool)(_events[i].flags & models::EVT_DONE);
            o["rem"]   = (bool)(_events[i].flags & models::EVT_REMINDER);
        }
    }
    serializeJson(doc, out);
}

// --- Internos ---------------------------------------------------------------

models::Event* CalendarStore::findById(uint32_t id) {
    for (int i = 0; i < _count; ++i) {
        if (_events[i].id == id) return &_events[i];
    }
    return nullptr;
}

void CalendarStore::bump() {
    _rev = _rev + 1;
    _dirtySave = true;
    _saveAt = millis() + SAVE_DEBOUNCE_MS;
}

void CalendarStore::saveNow() {
    _dirtySave = false;
    const bool ok = storage.atomicWrite(config::AGENDA_FILE, [this](File& f) {
        JsonDocument doc;
        JsonArray cals = doc["calendars"].to<JsonArray>();
        for (int i = 0; i < NUM_CALENDARS; ++i) {
            JsonObject o = cals.add<JsonObject>();
            o["id"]   = _cals[i].id;
            o["name"] = _cals[i].name;
        }
        JsonArray evs = doc["events"].to<JsonArray>();
        for (int i = 0; i < _count; ++i) {
            JsonObject o = evs.add<JsonObject>();
            o["id"]    = _events[i].id;
            o["title"] = _events[i].title;
            o["start"] = (uint32_t)_events[i].start;
            o["end"]   = (uint32_t)_events[i].end;
            o["cal"]   = _events[i].calendarId;
            o["alert"] = _events[i].alertMinBefore;
            o["flags"] = _events[i].flags;
        }
        return serializeJson(doc, f) > 0;
    });
    if (!ok) log_w("No se pudo guardar agenda.json");
}
