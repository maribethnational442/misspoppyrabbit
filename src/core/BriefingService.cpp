#include "BriefingService.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ctime>
#include "Config.h"
#include "Melody.h"

BriefingService briefingService;

namespace {
constexpr const char* KEY_MIN = "briefMin";
constexpr const char* KEY_DAY = "briefDay";

// "Buenos días": arpegio alegre ascendente, suena una sola vez
const Note MELODY_BRIEF[] = {
    {523, 100}, {659, 100}, {784, 100}, {1047, 120}, {1319, 280},
};
}

void BriefingService::begin() {
    Preferences prefs;
    prefs.begin(config::NVS_NS, /*readOnly=*/true);
    _briefMin = prefs.getUShort(KEY_MIN, 8 * 60);
    _lastShownKey = prefs.getUInt(KEY_DAY, 0);
    prefs.end();
    if (_briefMin >= 24 * 60) _briefMin = 8 * 60;
}

void BriefingService::loop() {
    if (_active) return;
    const uint32_t nowMs = millis();
    if (nowMs - _lastCheck < CHECK_EVERY_MS) return;
    _lastCheck = nowMs;

    const time_t now = time(nullptr);
    if (now < 1600000000) return;   // sin hora real todavía

    struct tm t;
    localtime_r(&now, &t);
    const uint32_t todayKey = (uint32_t)(t.tm_year + 1900) * 1000 + t.tm_yday;
    const int minOfDay = t.tm_hour * 60 + t.tm_min;

    if (minOfDay >= _briefMin && todayKey != _lastShownKey) {
        _lastShownKey = todayKey;
        Preferences prefs;
        prefs.begin(config::NVS_NS, false);
        prefs.putUInt(KEY_DAY, todayKey);
        prefs.end();

        _active = true;
        melodyPlayer.play(MELODY_BRIEF, sizeof(MELODY_BRIEF) / sizeof(Note), 0);
    }
}

void BriefingService::adjustBriefMin(int deltaMin) {
    _briefMin = (_briefMin + deltaMin + 24 * 60) % (24 * 60);
    Preferences prefs;
    prefs.begin(config::NVS_NS, false);
    prefs.putUShort(KEY_MIN, (uint16_t)_briefMin);
    prefs.end();
}
