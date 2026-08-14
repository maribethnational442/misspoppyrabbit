#pragma once
#include <ESPAsyncWebServer.h>

// ============================================================================
// WebService — la WebUI de Miss Poppy Rabbit servida desde el propio dispositivo.
//
// * Estáticos (index.html.gz) desde LittleFS en flash, servidos comprimidos.
// * REST: /api/status, /api/tasks (+ acciones), /api/pomodoro.
// * WebSocket /ws: empuja tareas y tick del pomodoro en vivo (máx 2 clientes:
//   cada conexión cuesta heap y no hay PSRAM).
//
// REGLA DE ORO de concurrencia: los handlers corren en la tarea async_tcp.
// Aquí NUNCA se muta estado del sistema directamente — solo se encola
// (taskStore.enqueue*, pomodoroService.request*) y se leen snapshots.
// Los broadcasts de WebSocket salen de loop() (loop principal).
// ============================================================================

class WebService {
public:
    void begin();   // define rutas; el servidor arranca al conectarse el WiFi
    void loop();    // arranque diferido, broadcasts, limpieza de clientes ws

private:
    void setupRoutes();
    void buildStatusJson(String& out);
    void buildPomodoroJson(String& out);
    void broadcastTasks();
    void broadcastPomodoro();

    AsyncWebServer  _server{80};
    AsyncWebSocket  _ws{"/ws"};
    bool _started = false;

    uint32_t _lastTaskRev = 0;
    uint32_t _lastPomoRev = 0;
    int      _lastPomoSec = -1;
    uint32_t _lastCleanup = 0;
};

extern WebService webService;
