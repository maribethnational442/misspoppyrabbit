#pragma once
#include <M5GFX.h>
#include "Theme.h"
#include "PixelArt.h"
#include "assets/PoppySprites.h"
#include "../core/Config.h"

// Pantalla de arranque: mascota grande + nombre + barra de progreso poppy.
// Es la única animación bloqueante del sistema (~1.5s): mientras tanto el
// WifiService ya está conectando en segundo plano.
inline void showBootScreen(M5Canvas& c) {
    using namespace theme;
    constexpr uint32_t DURATION_MS = 1500;
    const uint32_t t0 = millis();

    while (true) {
        const uint32_t elapsed = millis() - t0;
        if (elapsed >= DURATION_MS) break;
        const float p = (float)elapsed / DURATION_MS;

        c.fillSprite(BG);

        // Conejo 16×16 a escala 4 (64px), centrado; parpadea cerca del final
        const bool blink = (p > 0.60f && p < 0.75f);
        pixelart::draw(c, blink ? sprites::RABBIT_BLINK : sprites::RABBIT_IDLE,
                       sprites::RABBIT_H, (SCREEN_W - 64) / 2, 6, 4);

        c.setFont(&fonts::Font0);
        c.setTextDatum(textdatum_t::top_center);
        c.setTextSize(2);
        c.setTextColor(PRIMARY);
        c.drawString(config::OS_NAME, SCREEN_W / 2, 78);
        c.setTextSize(1);
        c.setTextColor(GRAY);
        c.drawString(config::OS_VERSION, SCREEN_W / 2, 98);

        // Barra de progreso
        const int barW = 100, barX = (SCREEN_W - barW) / 2, barY = 116;
        c.drawRect(barX, barY, barW, 5, DARKGRAY);
        c.fillRect(barX + 1, barY + 1, (int)((barW - 2) * p), 3, POPPY);

        c.pushSprite(0, 0);
        delay(16);
    }
}
