#include "StatusBar.h"
#include <M5Cardputer.h>
#include <ctime>
#include "../ui/Theme.h"
#include "WifiService.h"

void StatusBar::draw(M5Canvas& c) {
    using namespace theme;

    c.fillRect(0, 0, SCREEN_W, STATUSBAR_H, BG);
    c.drawFastHLine(0, STATUSBAR_H - 1, SCREEN_W, DARKGRAY);

    // --- Hora (izquierda). Hasta que NTP sincronice, time() devuelve ~1970:
    // lo detectamos por el año y mostramos "--:--".
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);
    time_t now = time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    char buf[8];
    if (tmNow.tm_year + 1900 >= 2020) {
        snprintf(buf, sizeof(buf), "%02d:%02d", tmNow.tm_hour, tmNow.tm_min);
        c.setTextColor(PRIMARY);
    } else {
        snprintf(buf, sizeof(buf), "--:--");
        c.setTextColor(GRAY);
    }
    c.drawString(buf, PADDING, 3);

    // --- Indicadores (derecha)
    drawBattery(c, SCREEN_W - 24, 3);
    drawWifi(c, SCREEN_W - 42, 3);
}

// Tres barras de señal. Verde = conectado, gris = conectando (parpadeo
// implícito al refrescar), gris oscuro = sin conexión.
void StatusBar::drawWifi(M5Canvas& c, int x, int y) {
    using namespace theme;
    uint16_t col = DARKGRAY;
    int bars = 0;
    switch (wifiService.state()) {
        case WifiService::State::Connected: {
            col = STEM;
            const int rssi = wifiService.rssi();
            bars = (rssi > -60) ? 3 : (rssi > -75) ? 2 : 1;
            break;
        }
        case WifiService::State::Connecting: col = GRAY; bars = 3; break;
        default: col = DARKGRAY; bars = 3; break;
    }
    for (int i = 0; i < 3; ++i) {
        const int h = 2 + i * 3;                     // 2, 5, 8 px de alto
        const uint16_t barCol = (i < bars) ? col : DARKGRAY;
        c.fillRect(x + i * 4, y + 8 - h, 3, h, barCol);
    }
}

void StatusBar::drawBattery(M5Canvas& c, int x, int y) {
    using namespace theme;
    const int level = M5Cardputer.Power.getBatteryLevel();   // 0..100

    // Color según carga: verde OK, blanco medio, poppy = crítico
    const uint16_t col = (level > 40) ? STEM : (level > 20) ? PRIMARY : POPPY;

    c.drawRect(x, y, 18, 8, PRIMARY);                // cuerpo
    c.fillRect(x + 18, y + 2, 2, 4, PRIMARY);        // borne
    const int w = (level * 14) / 100;                // relleno proporcional
    if (w > 0) c.fillRect(x + 2, y + 2, w, 4, col);
}
