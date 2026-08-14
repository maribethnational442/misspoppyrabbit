#include "TaskRepo.h"
#include <ArduinoJson.h>
#include <SD.h>
#include <cstring>
#include "../core/StorageService.h"
#include "../core/Config.h"

namespace taskrepo {

int load(models::Task* out, int maxTasks) {
    if (!storage.mounted()) return -1;
    SdLock lock;

    File f = SD.open(config::TASKS_FILE, FILE_READ);
    if (!f) return 0;   // primera ejecución: aún no hay archivo

    // ArduinoJson deserializa en streaming desde el File: el JSON crudo
    // nunca se copia entero a RAM, solo el árbol parseado (~100B/tarea).
    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        log_e("tasks.json corrupto: %s", err.c_str());
        return -2;
    }

    int n = 0;
    for (JsonObject o : doc["tasks"].as<JsonArray>()) {
        if (n >= maxTasks) break;
        models::Task& t = out[n++];
        t.id = o["id"] | 0u;
        strncpy(t.title, o["title"] | "", sizeof(t.title) - 1);
        t.title[sizeof(t.title) - 1] = '\0';
        t.due        = (time_t)(o["due"] | 0u);
        t.calendarId = o["cal"]   | (uint8_t)0;
        t.priority   = o["prio"]  | (uint8_t)1;
        t.flags      = o["flags"] | (uint8_t)0;
    }
    return n;
}

bool save(const models::Task* tasks, int count) {
    return storage.atomicWrite(config::TASKS_FILE, [&](File& f) {
        JsonDocument doc;
        doc["version"] = 1;
        JsonArray arr = doc["tasks"].to<JsonArray>();
        for (int i = 0; i < count; ++i) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]    = tasks[i].id;
            o["title"] = tasks[i].title;
            o["due"]   = (uint32_t)tasks[i].due;
            o["cal"]   = tasks[i].calendarId;
            o["prio"]  = tasks[i].priority;
            o["flags"] = tasks[i].flags;
        }
        return serializeJson(doc, f) > 0;   // serializa directo al archivo
    });
}

} // namespace taskrepo
