#include "Sound.h"
#include <Preferences.h>
#include "Config.h"

namespace {
int g_volumePct = 60;   // por defecto: audible sin asustar
constexpr const char* NVS_KEY = "vol";

void apply() {
    // El speaker trabaja en 0-255; nuestro mando en 0-100%
    M5Cardputer.Speaker.setVolume((uint8_t)((g_volumePct * 255) / 100));
}
}

namespace sound {

void begin() {
    Preferences prefs;
    prefs.begin(config::NVS_NS, /*readOnly=*/true);
    g_volumePct = prefs.getUChar(NVS_KEY, 60);
    prefs.end();
    if (g_volumePct > 100) g_volumePct = 100;
    apply();
}

void setVolumePct(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    g_volumePct = pct;
    apply();
    Preferences prefs;
    prefs.begin(config::NVS_NS, false);
    prefs.putUChar(NVS_KEY, (uint8_t)pct);
    prefs.end();
    click();   // muestra inmediata: "así de fuerte suena ahora"
}

int volumePct() { return g_volumePct; }

} // namespace sound
