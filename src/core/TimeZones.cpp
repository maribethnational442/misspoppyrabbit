#include "TimeZones.h"
#include <Preferences.h>
#include <cstdlib>
#include <ctime>
#include "Config.h"

namespace {

struct Zone {
    const char* label;
    const char* posix;
};

// Los nombres raros tipo "<-05>5" son el formato POSIX para zonas sin
// abreviatura oficial; las reglas M3.2.0 etc. codifican el horario de verano.
const Zone ZONES[] = {
    {"Bogota",       "<-05>5"},
    {"Mexico City",  "CST6"},
    {"Buenos Aires", "<-03>3"},
    {"Santiago",     "<-04>4<-03>,M9.1.6/24,M4.1.6/24"},
    {"New York",     "EST5EDT,M3.2.0,M11.1.0"},
    {"Los Angeles",  "PST8PDT,M3.2.0,M11.1.0"},
    {"Madrid",       "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"London",       "GMT0BST,M3.5.0/1,M10.5.0"},
    {"UTC",          "UTC0"},
};
constexpr int NUM_ZONES = sizeof(ZONES) / sizeof(ZONES[0]);

int g_idx = 0;   // defecto: Bogotá
constexpr const char* NVS_KEY = "tz";

void apply() {
    setenv("TZ", ZONES[g_idx].posix, 1);
    tzset();   // a partir de aquí localtime() usa la zona nueva
}
}

namespace tzones {

void begin() {
    Preferences prefs;
    prefs.begin(config::NVS_NS, /*readOnly=*/true);
    g_idx = prefs.getUChar(NVS_KEY, 0) % NUM_ZONES;
    prefs.end();
    apply();
}

void cycle(int dir) {
    g_idx = (g_idx + dir + NUM_ZONES) % NUM_ZONES;
    apply();
    Preferences prefs;
    prefs.begin(config::NVS_NS, false);
    prefs.putUChar(NVS_KEY, (uint8_t)g_idx);
    prefs.end();
}

const char* label() { return ZONES[g_idx].label; }
const char* posix() { return ZONES[g_idx].posix; }

} // namespace tzones
