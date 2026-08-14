#include "SettingsApp.h"
#include "../core/AppManager.h"
#include "../core/Sound.h"
#include "../ui/Theme.h"
#include "../ui/assets/Icons.h"
#include <WiFi.h>
#include <cstring>

namespace {
constexpr int HEADER_Y = 18;
constexpr int LIST_Y   = 34;
constexpr int ROW_H    = 13;
}

const char* const* SettingsApp::icon() const { return icons::GEAR; }

void SettingsApp::onEnter() {
    _mode = Mode::List;
    _nav.reset();
    _nav.visible = 6;
    _lastState = wifiService.state();
    _headerScroll.reset();
}

void SettingsApp::update(uint32_t dtMs) {
    (void)dtMs;

    // ¿Terminó el escaneo asíncrono? Copiamos los resultados a nuestros
    // buffers fijos y liberamos los del driver WiFi (que viven en heap).
    if (wifiService.scanning()) {
        const int n = wifiService.scanResult();
        if (n >= 0) {
            _netCount = 0;
            for (int i = 0; i < n && _netCount < MAX_NETS; ++i) {
                Net& net = _nets[_netCount++];
                strncpy(net.ssid, WiFi.SSID(i).c_str(), sizeof(net.ssid) - 1);
                net.ssid[sizeof(net.ssid) - 1] = '\0';
                net.rssi = WiFi.RSSI(i);
                net.open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
            }
            WiFi.scanDelete();   // devuelve al heap la lista interna del driver
            requestRedraw();
        }
    }

    // Repintar cuando cambia el estado del WiFi (p.ej. Conectando → Conectado)
    if (wifiService.state() != _lastState) {
        _lastState = wifiService.state();
        _headerScroll.reset();   // texto nuevo: el scroll empieza de cero
        requestRedraw();
    }

    // Marquee de la cabecera: mientras se desliza pide repintado
    if (_mode == Mode::List && _headerScroll.tick(dtMs)) requestRedraw();
}

// Filas: [0] Buscar redes | [1] Idioma | [2] Volumen | [3] Olvidar red | [4..] redes
int SettingsApp::rowCount() const {
    return 3 + (wifiService.hasCredentials() ? 1 : 0) + _netCount;
}

void SettingsApp::activateRow(int row) {
    const int netBase = 3 + (wifiService.hasCredentials() ? 1 : 0);

    if (row == 0) {
        if (!wifiService.scanning()) wifiService.startScan();
        requestRedraw();
        return;
    }
    if (row == 1) {
        // Alternar EN ↔ ES; todo se repinta con tr() así que basta redibujar
        lang::set(lang::current() == lang::Code::EN ? lang::Code::ES
                                                    : lang::Code::EN);
        requestRedraw();
        return;
    }
    if (row == 2) {
        // ENTER también sirve: cicla 0→25→50→75→100→0
        sound::setVolumePct(sound::volumePct() >= 100 ? 0 : sound::volumePct() + 25);
        requestRedraw();
        return;
    }
    if (wifiService.hasCredentials() && row == 3) {
        wifiService.forget();
        _nav.reset();
        requestRedraw();
        return;
    }
    const int idx = row - netBase;
    if (idx < 0 || idx >= _netCount) return;

    if (_nets[idx].open) {
        wifiService.connectTo(_nets[idx].ssid, "", /*save=*/true);
    } else {
        _targetNet = idx;
        _pass.clear();
        _mode = Mode::Password;
    }
    requestRedraw();
}

void SettingsApp::draw(M5Canvas& c) {
    if (_mode == Mode::Password) drawPassword(c);
    else drawList(c);
}

void SettingsApp::drawList(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    // Cabecera de estado
    char header[64];
    uint16_t headerCol = GRAY;
    switch (wifiService.state()) {
        case WifiService::State::Connected:
            snprintf(header, sizeof(header), "WiFi: %s (%s)",
                     wifiService.ssid(), wifiService.ip().c_str());
            headerCol = STEM;
            break;
        case WifiService::State::Connecting:
            snprintf(header, sizeof(header), tr(Str::WifiConnectingFmt), wifiService.ssid());
            break;
        case WifiService::State::Failed:
            snprintf(header, sizeof(header), tr(Str::WifiFailedFmt), wifiService.ssid());
            headerCol = POPPY;
            break;
        default:
            snprintf(header, sizeof(header), "%s", tr(Str::WifiNone));
            break;
    }
    c.setTextColor(headerCol);
    _headerScroll.draw(c, header, PADDING, HEADER_Y, SCREEN_W - 2 * PADDING);
    c.drawFastHLine(0, LIST_Y - 4, SCREEN_W, DARKGRAY);

    // Lista con scroll
    const int netBase = 3 + (wifiService.hasCredentials() ? 1 : 0);
    for (int v = 0; v < _nav.visible; ++v) {
        const int row = _nav.scroll + v;
        if (row >= rowCount()) break;
        const int y = LIST_Y + v * ROW_H;
        const bool selected = (row == _nav.sel);

        if (selected) c.fillRoundRect(2, y - 2, SCREEN_W - 4, ROW_H, 3, POPPY_DIM);
        c.setTextColor(selected ? PRIMARY : GRAY);

        char line[48];
        if (row == 0) {
            snprintf(line, sizeof(line), "%s",
                     wifiService.scanning() ? tr(Str::ScanningRow) : tr(Str::ScanRow));
        } else if (row == 1) {
            snprintf(line, sizeof(line), tr(Str::LanguageRowFmt), lang::displayName());
        } else if (row == 2) {
            snprintf(line, sizeof(line), tr(Str::VolumeRowFmt), sound::volumePct());
        } else if (wifiService.hasCredentials() && row == 3) {
            snprintf(line, sizeof(line), tr(Str::ForgetRowFmt), wifiService.ssid());
        } else {
            const Net& net = _nets[row - netBase];
            snprintf(line, sizeof(line), "%s%-22.22s %4d dBm",
                     net.open ? " " : "*", net.ssid, net.rssi);
        }
        c.drawString(line, PADDING + 2, y);
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::SettingsHint), PADDING, SCREEN_H - 2);
}

void SettingsApp::drawPassword(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    c.setTextColor(PRIMARY);
    char title[48];
    snprintf(title, sizeof(title), tr(Str::PasswordTitleFmt), _nets[_targetNet].ssid);
    c.drawString(title, PADDING, 30);

    _pass.draw(c, PADDING, 44, SCREEN_W - 2 * PADDING);

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::PasswordHint), PADDING, SCREEN_H - 2);
}

void SettingsApp::onKey(const KeyEvent& e) {
    if (_mode == Mode::Password) {
        if (_pass.handleKey(e)) {
            requestRedraw();
            return;
        }
        if (e.key == Key::Ok) {
            wifiService.connectTo(_nets[_targetNet].ssid, _pass.buf, /*save=*/true);
            _mode = Mode::List;
            requestRedraw();
        } else if (e.key == Key::Back) {
            _mode = Mode::List;
            requestRedraw();
        }
        return;
    }

    // Modo lista
    if (_nav.onKey(e, rowCount())) {
        requestRedraw();
        return;
    }
    switch (e.key) {
        case Key::Ok:
            activateRow(_nav.sel);
            break;
        case Key::Left:   // en la fila de volumen, , / ajustan en pasos de 5
            if (_nav.sel == 2) {
                sound::setVolumePct(sound::volumePct() - 5);
                requestRedraw();
            }
            break;
        case Key::Right:
            if (_nav.sel == 2) {
                sound::setVolumePct(sound::volumePct() + 5);
                requestRedraw();
            }
            break;
        case Key::Back:
            appManager.goBack();
            break;
        default:
            break;
    }
}
