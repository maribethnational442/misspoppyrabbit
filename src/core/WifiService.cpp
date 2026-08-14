#include "WifiService.h"
#include <Preferences.h>
#include "Config.h"
#include "TimeZones.h"

WifiService wifiService;

namespace {
constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t RETRY_INTERVAL_MS  = 30000;
constexpr const char* KEY_SSID = "wifi_ssid";
constexpr const char* KEY_PASS = "wifi_pass";
}

void WifiService::begin() {
    // Leemos credenciales de NVS (flash de configuración). Preferences se
    // abre y cierra en el momento: no merece la pena tenerlo abierto siempre.
    Preferences prefs;
    prefs.begin(config::NVS_NS, /*readOnly=*/true);
    prefs.getString(KEY_SSID, _ssid, sizeof(_ssid));
    prefs.getString(KEY_PASS, _pass, sizeof(_pass));
    prefs.end();

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(config::HOSTNAME);
    WiFi.setAutoReconnect(false);   // la reconexión la gestiona nuestra FSM

    if (hasCredentials()) beginConnect();
}

void WifiService::loop() {
    switch (_state) {
        case State::Connecting:
            if (WiFi.status() == WL_CONNECTED) {
                _state = State::Connected;
                log_i("WiFi conectado a %s, IP %s", _ssid, ip().c_str());
                syncClock();
            } else if (millis() - _connectStart > CONNECT_TIMEOUT_MS) {
                log_w("WiFi: timeout conectando a %s", _ssid);
                WiFi.disconnect();
                _state = State::Failed;
                _retryAt = millis() + RETRY_INTERVAL_MS;
            }
            break;

        case State::Connected:
            if (WiFi.status() != WL_CONNECTED) {
                log_w("WiFi: conexion perdida, reintentando");
                beginConnect();
            }
            break;

        case State::Failed:
            // Reintento suave: cada 30s mientras haya credenciales guardadas.
            if (hasCredentials() && millis() > _retryAt) beginConnect();
            break;

        case State::Idle:
            break;
    }
}

void WifiService::connectTo(const char* ssid, const char* pass, bool save) {
    strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    _ssid[sizeof(_ssid) - 1] = '\0';
    strncpy(_pass, pass, sizeof(_pass) - 1);
    _pass[sizeof(_pass) - 1] = '\0';

    if (save) {
        Preferences prefs;
        prefs.begin(config::NVS_NS, false);
        prefs.putString(KEY_SSID, _ssid);
        prefs.putString(KEY_PASS, _pass);
        prefs.end();
    }
    beginConnect();
}

void WifiService::forget() {
    Preferences prefs;
    prefs.begin(config::NVS_NS, false);
    prefs.remove(KEY_SSID);
    prefs.remove(KEY_PASS);
    prefs.end();
    _ssid[0] = '\0';
    _pass[0] = '\0';
    WiFi.disconnect();
    _state = State::Idle;
}

void WifiService::beginConnect() {
    log_i("WiFi: conectando a %s", _ssid);
    WiFi.begin(_ssid, _pass);
    _state = State::Connecting;
    _connectStart = millis();
}

void WifiService::startScan() {
    // async=true: la radio escanea en segundo plano y la UI sigue fluida.
    WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/false);
    _scanning = true;
}

int WifiService::scanResult() {
    const int n = WiFi.scanComplete();   // -1 en curso, -2 sin escaneo
    if (n >= 0) _scanning = false;
    return n;
}

void WifiService::syncClock() {
    // NTP con la zona elegida en Settings: time() dará la hora local real.
    configTzTime(tzones::posix(), config::NTP_SERVER);
}
