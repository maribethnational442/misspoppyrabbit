#pragma once
#include <cstdint>

// ============================================================================
// PomodoroService — el temporizador como servicio del sistema (mismo patrón
// que WifiService/TaskStore). PomodoroApp es solo su cara en pantalla, y la
// WebUI otra. Al vivir aquí, loop() corre SIEMPRE: el beep de fin de tramo
// suena aunque estés en el launcher o mirando las tareas — arregla la
// limitación que teníamos en v0.2.
//
// Los getters son lecturas de enteros de 32 bits (atómicas en el ESP32), así
// que la tarea del servidor web puede consultarlos sin lock. Los comandos de
// la web sí van por bandera → los ejecuta el loop principal (un solo escritor,
// como en TaskStore).
// ============================================================================

class PomodoroService {
public:
    enum class Phase : uint8_t { Work, Break };
    enum class Run   : uint8_t { Ready, Running, Paused, Finished };

    void loop();

    // --- Comandos (loop principal: la app en pantalla) ---
    void primaryAction();          // el "ENTER": empezar/pausar/seguir/siguiente
    void reset();
    void adjustWork(int deltaMin); // solo en Ready+Work; rango 5..60

    // --- Comandos desde la web (otra tarea): bandera, no ejecución directa ---
    void requestPrimaryAction() { _webCmd = 1; }
    void requestReset()         { _webCmd = 2; }

    // --- Estado (legible desde cualquier tarea) ---
    Phase phase() const      { return _phase; }
    Run   run() const        { return _run; }
    int   workMin() const    { return _workMin; }
    int   todayCount() const { return _todayCount; }
    uint32_t totalMs() const;
    uint32_t remainingMs() const;
    // cambia con cada transición de estado (para saber cuándo repintar/emitir)
    uint32_t revision() const { return _rev; }

private:
    void startPhase();
    void finishPhase();
    void bump() { _rev = _rev + 1; }

    Phase _phase = Phase::Work;
    Run   _run = Run::Ready;
    int   _workMin = 25;
    int   _breakMin = 5;
    uint32_t _endAt = 0;
    uint32_t _pausedRemain = 0;
    int   _todayCount = 0;
    volatile uint32_t _rev = 1;
    volatile uint8_t  _webCmd = 0;
};

extern PomodoroService pomodoroService;
