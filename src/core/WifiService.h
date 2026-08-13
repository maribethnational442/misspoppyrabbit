#pragma once
#include <WiFi.h>

// ============================================================================
// WifiService — el WiFi como SERVICIO del sistema, no como parte de una app.
//
// Razones: la reconexión automática al arrancar no puede depender de que
// Settings esté abierta, y la StatusBar pregunta el estado desde cualquier
// pantalla. SettingsApp es solo la interfaz de usuario de este servicio.
//
// Máquina de estados no bloqueante (loop() nunca espera):
//   Idle → Connecting → Connected
//                  ↘ Failed → (reintento cada 30s) → Connecting
// ============================================================================

class WifiService {
public:
    enum class State : uint8_t { Idle, Connecting, Connected, Failed };

    void begin();    // carga credenciales de NVS y arranca la conexión si hay
    void loop();     // avanza la máquina de estados; llamar en cada loop()

    // Conecta a una red; con save=true persiste las credenciales en NVS.
    void connectTo(const char* ssid, const char* pass, bool save);
    void forget();   // borra credenciales y desconecta

    State state() const          { return _state; }
    bool  hasCredentials() const { return _ssid[0] != '\0'; }
    const char* ssid() const     { return _ssid; }
    int   rssi() const           { return WiFi.RSSI(); }
    String ip() const            { return WiFi.localIP().toString(); }

    // --- Escaneo asíncrono (el bloqueante congelaría la UI 2-3 segundos) ---
    void startScan();
    // <0 = en curso o sin escaneo; >=0 = número de redes encontradas
    int  scanResult();
    bool scanning() const        { return _scanning; }

private:
    void beginConnect();
    void syncClock();            // NTP: la hora de la StatusBar y el Reloj

    char     _ssid[33] = {0};    // 32 chars máx de SSID + terminador
    char     _pass[65] = {0};    // 64 máx de WPA + terminador
    State    _state = State::Idle;
    uint32_t _connectStart = 0;
    uint32_t _retryAt = 0;
    bool     _scanning = false;
};

extern WifiService wifiService;
