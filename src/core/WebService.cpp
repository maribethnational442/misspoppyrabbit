#include "WebService.h"
#include <AsyncJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <M5Cardputer.h>
#include "CalendarStore.h"
#include "Config.h"
#include "Lang.h"
#include "TaskStore.h"
#include "PomodoroService.h"
#include "WifiService.h"

WebService webService;

namespace {
constexpr int MAX_WS_CLIENTS = 2;   // cada cliente cuesta heap; sin PSRAM, 2 bastan

const char* pomoPhaseStr(PomodoroService::Phase p) {
    return (p == PomodoroService::Phase::Work) ? "work" : "break";
}
const char* pomoRunStr(PomodoroService::Run r) {
    switch (r) {
        case PomodoroService::Run::Running:  return "running";
        case PomodoroService::Run::Paused:   return "paused";
        case PomodoroService::Run::Finished: return "finished";
        default:                             return "ready";
    }
}

// id llega como query param (?id=N). OJO: strtoul y no toInt() — los ids
// importados son hashes de 32 bits SIN signo y toInt() desborda con la
// mitad de ellos (bug real encontrado en pruebas).
uint32_t idParam(AsyncWebServerRequest* req) {
    if (!req->hasParam("id")) return 0;
    return (uint32_t)strtoul(req->getParam("id")->value().c_str(), nullptr, 10);
}

// Hash FNV-1a de 32 bits: convierte el UID textual de un evento externo
// ("abc123@google.com") en nuestro id numérico ESTABLE. Mismo UID → mismo
// id siempre → re-importar actualiza en vez de duplicar (idempotencia).
uint32_t fnv1a(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (uint8_t)*s++;
        h *= 16777619u;
    }
    return (h != 0) ? h : 1u;   // 0 significa "crear nuevo": lo evitamos
}
}

void WebService::begin() {
    if (!LittleFS.begin(true)) {   // true = formatear si está virgen
        log_w("LittleFS no disponible: la WebUI no tendra frontend");
    }
    // CORS abierto: la extensión de Chrome (v0.5) postea desde otro origen
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    setupRoutes();
    // Ojo: _server.begin() NO se llama aquí — esperamos al WiFi en loop()
}

void WebService::loop() {
    // Arranque diferido: el servidor y mDNS necesitan la red levantada
    if (!_started && wifiService.state() == WifiService::State::Connected) {
        MDNS.begin(config::HOSTNAME);
        MDNS.addService("http", "tcp", 80);
        _server.begin();
        _started = true;
        log_i("WebUI en http://%s.local (%s)", config::HOSTNAME, wifiService.ip().c_str());
    }
    if (!_started) return;

    // Empujar cambios a los navegadores conectados (desde el loop principal)
    if (_ws.count() > 0) {
        if (taskStore.revision() != _lastTaskRev) {
            _lastTaskRev = taskStore.revision();
            broadcastTasks();
        }
        if (calendarStore.revision() != _lastAgendaRev) {
            _lastAgendaRev = calendarStore.revision();
            broadcastAgenda();
        }
        const bool running = pomodoroService.run() == PomodoroService::Run::Running;
        const int sec = (int)(pomodoroService.remainingMs() / 1000);
        if (pomodoroService.revision() != _lastPomoRev || (running && sec != _lastPomoSec)) {
            _lastPomoRev = pomodoroService.revision();
            _lastPomoSec = sec;
            broadcastPomodoro();
        }
    }

    // Limpieza periódica de sockets muertos (recomendación de la lib)
    if (millis() - _lastCleanup > 1000) {
        _lastCleanup = millis();
        _ws.cleanupClients(MAX_WS_CLIENTS);
    }
}

void WebService::setupRoutes() {
    // --- WebSocket -----------------------------------------------------------
    _ws.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client,
                       AwsEventType type, void*, uint8_t*, size_t) {
        if (type == WS_EVT_CONNECT) {
            if (_ws.count() > MAX_WS_CLIENTS) {
                client->close();   // lo siento: sin PSRAM no hay fiesta grande
                return;
            }
            // Snapshot inicial para que el navegador pinte sin esperar
            String tasks;
            taskStore.snapshotJson(tasks);
            client->text(String("{\"type\":\"tasks\",\"tasks\":") + tasks + "}");
            String pomo;
            buildPomodoroJson(pomo);
            client->text(String("{\"type\":\"pomo\",\"pomo\":") + pomo + "}");
            // La agenda NO viaja por WS (puede pesar ~40KB con 300 eventos):
            // se avisa "sucia" y el navegador la pide por GET /api/events.
            client->text("{\"type\":\"agendaDirty\"}");
        }
    });
    _server.addHandler(&_ws);

    // --- API -----------------------------------------------------------------
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        String out;
        buildStatusJson(out);
        req->send(200, "application/json", out);
    });

    // OJO al orden: ESPAsyncWebServer evalúa los handlers en orden de registro
    // y "/api/tasks" hace match POR PREFIJO con "/api/tasks/toggle". Las rutas
    // específicas van PRIMERO o la genérica se las traga (bug real que tuvimos).
    _server.on("/api/tasks/toggle", HTTP_POST, [](AsyncWebServerRequest* req) {
        const bool ok = taskStore.enqueueToggle(idParam(req));
        req->send(ok ? 202 : 503, "application/json", "{}");
    });
    _server.on("/api/tasks/delete", HTTP_POST, [](AsyncWebServerRequest* req) {
        const bool ok = taskStore.enqueueRemove(idParam(req));
        req->send(ok ? 202 : 503, "application/json", "{}");
    });
    _server.on("/api/tasks/priority", HTTP_POST, [](AsyncWebServerRequest* req) {
        const bool ok = taskStore.enqueuePriority(idParam(req));
        req->send(ok ? 202 : 503, "application/json", "{}");
    });

    _server.on("/api/tasks", HTTP_GET, [](AsyncWebServerRequest* req) {
        String out;
        taskStore.snapshotJson(out);
        req->send(200, "application/json", out);
    });

    // Crear tarea. El título viaja como query param (?title=...) para no
    // necesitar parser de body: simple y suficiente para una UI personal.
    _server.on("/api/tasks", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("title", true) && !req->hasParam("title")) {
            req->send(400, "application/json", "{\"error\":\"falta title\"}");
            return;
        }
        const AsyncWebParameter* p = req->hasParam("title", true)
                                         ? req->getParam("title", true)
                                         : req->getParam("title");
        const bool ok = taskStore.enqueueAdd(p->value().c_str());
        req->send(ok ? 202 : 503, "application/json",
                  ok ? "{\"queued\":true}" : "{\"error\":\"cola llena\"}");
    });

    // --- Agenda (rutas específicas primero, como siempre) --------------------

    // Import masivo: MISMO contrato para el .ics parseado por la WebUI y para
    // la extensión de Chrome (docs/IMPORT_API.md). Body JSON:
    //   {"calendarId":0,"events":[{"uid":"...","title":"...","start":e,"end":e}]}
    // OJO: registrado ANTES que POST /api/events (match por prefijo).
    auto* importHandler = new AsyncCallbackJsonWebHandler(
        "/api/events/import",
        [](AsyncWebServerRequest* req, JsonVariant& json) {
            const JsonObject root = json.as<JsonObject>();
            const uint8_t defCal = (uint8_t)((int)(root["calendarId"] | 0) % 4);
            int queued = 0, dropped = 0;
            for (JsonObject o : root["events"].as<JsonArray>()) {
                const char* uid = o["uid"] | "";
                const char* title = o["title"] | "";
                models::Event e = {};
                e.start = (time_t)(o["start"] | 0u);
                e.end   = (time_t)(o["end"] | 0u);
                if (uid[0] == '\0' || title[0] == '\0' || e.end <= e.start) {
                    ++dropped;
                    continue;
                }
                e.id = fnv1a(uid);
                strncpy(e.title, title, sizeof(e.title) - 1);
                e.calendarId = o["cal"].is<int>() ? (uint8_t)((int)o["cal"] % 4) : defCal;
                e.alertMinBefore = 10;
                e.flags = models::EVT_SYNCED;
                if (calendarStore.enqueueUpsertWait(e, 200)) ++queued;
                else ++dropped;
            }
            // "free" permite al cliente avisar si el store se está llenando
            // (con el store lleno, los upserts nuevos fallan en silencio)
            char resp[80];
            snprintf(resp, sizeof(resp), "{\"queued\":%d,\"dropped\":%d,\"free\":%d}",
                     queued, dropped,
                     CalendarStore::MAX_EVENTS - calendarStore.count());
            req->send(200, "application/json", resp);
        });
    importHandler->setMethod(HTTP_POST);
    _server.addHandler(importHandler);

    _server.on("/api/events/delete", HTTP_POST, [](AsyncWebServerRequest* req) {
        const bool ok = calendarStore.enqueueRemove(idParam(req));
        req->send(ok ? 202 : 503, "application/json", "{}");
    });

    _server.on("/api/events", HTTP_GET, [](AsyncWebServerRequest* req) {
        String out;
        calendarStore.snapshotJson(out);
        req->send(200, "application/json", out);
    });

    // Crear/editar por query params: title, start, end (epoch seg), cal, [id]
    _server.on("/api/events", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("title") || !req->hasParam("start") || !req->hasParam("end")) {
            req->send(400, "application/json", "{\"error\":\"title/start/end\"}");
            return;
        }
        models::Event e = {};
        e.id = req->hasParam("id") ? (uint32_t)req->getParam("id")->value().toInt() : 0;
        strncpy(e.title, req->getParam("title")->value().c_str(), sizeof(e.title) - 1);
        e.start = (time_t)strtoul(req->getParam("start")->value().c_str(), nullptr, 10);
        e.end   = (time_t)strtoul(req->getParam("end")->value().c_str(), nullptr, 10);
        e.calendarId = req->hasParam("cal")
                           ? (uint8_t)(req->getParam("cal")->value().toInt() % 4) : 0;
        e.alertMinBefore = 10;
        if (req->hasParam("rem") && req->getParam("rem")->value() == "1") {
            e.flags = models::EVT_REMINDER;
            e.end = e.start + 3600;   // ventana del nag; los reminders no duran
        }
        const bool ok = (e.end > e.start) && calendarStore.enqueueUpsert(e);
        req->send(ok ? 202 : 400, "application/json", "{}");
    });

    _server.on("/api/calendars/rename", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("id") || !req->hasParam("name")) {
            req->send(400, "application/json", "{}");
            return;
        }
        const bool ok = calendarStore.enqueueRename(
            (uint8_t)req->getParam("id")->value().toInt(),
            req->getParam("name")->value().c_str());
        req->send(ok ? 202 : 503, "application/json", "{}");
    });

    _server.on("/api/pomodoro", HTTP_GET, [this](AsyncWebServerRequest* req) {
        String out;
        buildPomodoroJson(out);
        req->send(200, "application/json", out);
    });
    _server.on("/api/pomodoro/action", HTTP_POST, [](AsyncWebServerRequest* req) {
        pomodoroService.requestPrimaryAction();
        req->send(202, "application/json", "{}");
    });
    _server.on("/api/pomodoro/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        pomodoroService.requestReset();
        req->send(202, "application/json", "{}");
    });

    // --- Estáticos: index.html.gz se sirve solo con Content-Encoding: gzip ---
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    _server.onNotFound([](AsyncWebServerRequest* req) {
        if (req->method() == HTTP_OPTIONS) {   // preflight CORS de la extensión
            AsyncWebServerResponse* r = req->beginResponse(204);
            r->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
            r->addHeader("Access-Control-Allow-Headers", "Content-Type");
            req->send(r);
            return;
        }
        req->send(404, "text/plain", "404 - el conejo no encontro eso");
    });
}

void WebService::buildStatusJson(String& out) {
    char buf[224];
    snprintf(buf, sizeof(buf),
             "{\"name\":\"%s\",\"version\":\"%s\",\"lang\":\"%s\",\"heap\":%u,\"battery\":%d,"
             "\"ssid\":\"%s\",\"ip\":\"%s\",\"uptimeMin\":%u,\"wsClients\":%u}",
             config::OS_NAME, config::OS_VERSION, lang::code(),
             (unsigned)ESP.getFreeHeap(),
             M5Cardputer.Power.getBatteryLevel(),
             wifiService.ssid(), wifiService.ip().c_str(),
             (unsigned)(millis() / 60000),
             (unsigned)_ws.count());
    out = buf;
}

void WebService::buildPomodoroJson(String& out) {
    char buf[160];
    snprintf(buf, sizeof(buf),
             "{\"phase\":\"%s\",\"run\":\"%s\",\"remainingSec\":%u,"
             "\"totalSec\":%u,\"workMin\":%d,\"today\":%d}",
             pomoPhaseStr(pomodoroService.phase()),
             pomoRunStr(pomodoroService.run()),
             (unsigned)(pomodoroService.remainingMs() / 1000),
             (unsigned)(pomodoroService.totalMs() / 1000),
             pomodoroService.workMin(),
             pomodoroService.todayCount());
    out = buf;
}

void WebService::broadcastTasks() {
    String tasks;
    taskStore.snapshotJson(tasks);
    _ws.textAll(String("{\"type\":\"tasks\",\"tasks\":") + tasks + "}");
}

void WebService::broadcastPomodoro() {
    String pomo;
    buildPomodoroJson(pomo);
    _ws.textAll(String("{\"type\":\"pomo\",\"pomo\":") + pomo + "}");
}

void WebService::broadcastAgenda() {
    // Ping ligero: cada cliente re-pide GET /api/events cuando le convenga
    _ws.textAll("{\"type\":\"agendaDirty\"}");
}
