#include "PomodoroApp.h"
#include "../core/AppManager.h"
#include "../core/PomodoroService.h"
#include "../ui/Theme.h"
#include "../ui/assets/Icons.h"

using Phase = PomodoroService::Phase;
using Run   = PomodoroService::Run;

namespace {
// Centro y radios del anillo (mitad izquierda de la pantalla)
constexpr int RING_X  = 64;
constexpr int RING_Y  = 74;
constexpr int RING_R0 = 36;
constexpr int RING_R1 = 48;
constexpr int PANEL_X = 130;

// Arco de progreso desde las 12 en punto, en sentido horario.
// En M5GFX los ángulos van en grados con 0° a las 3 en punto, así que las
// 12 son 270°; si el barrido cruza 360° hay que partirlo en dos arcos.
void drawProgressRing(M5Canvas& c, float progress, uint16_t color) {
    using namespace theme;
    c.fillArc(RING_X, RING_Y, RING_R0, RING_R1, 0, 360, DARKGRAY);
    if (progress <= 0.0f) return;
    const float sweep = 360.0f * ((progress > 1.0f) ? 1.0f : progress);
    const float a1 = 270.0f + sweep;
    if (a1 <= 360.0f) {
        c.fillArc(RING_X, RING_Y, RING_R0, RING_R1, 270.0f, a1, color);
    } else {
        c.fillArc(RING_X, RING_Y, RING_R0, RING_R1, 270.0f, 360.0f, color);
        c.fillArc(RING_X, RING_Y, RING_R0, RING_R1, 0.0f, a1 - 360.0f, color);
    }
}
}

const char* const* PomodoroApp::icon() const { return icons::POMODORO; }

void PomodoroApp::update(uint32_t dtMs) {
    (void)dtMs;
    // Repintar si el servicio cambió de estado (incluso por un comando web)
    if (pomodoroService.revision() != _lastRev) {
        _lastRev = pomodoroService.revision();
        requestRedraw();
        return;
    }
    // ...o si cambió el segundo visible del contador
    if (pomodoroService.run() == Run::Running) {
        const int sec = (int)(pomodoroService.remainingMs() / 1000);
        if (sec != _lastShownSec) {
            _lastShownSec = sec;
            requestRedraw();
        }
    }
}

void PomodoroApp::draw(M5Canvas& c) {
    using namespace theme;
    const bool work = (pomodoroService.phase() == Phase::Work);
    const uint16_t phaseCol = work ? POPPY : STEM;
    const Run run = pomodoroService.run();

    // Anillo: se va VACIANDO con el tiempo (lleno = todo por delante)
    const float progress = (float)pomodoroService.remainingMs() / (float)pomodoroService.totalMs();
    drawProgressRing(c, progress, phaseCol);

    // Tiempo restante en el centro
    const uint32_t rem = pomodoroService.remainingMs();
    char buf[8];
    snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)(rem / 60000), (unsigned)((rem / 1000) % 60));
    c.setFont(&fonts::Font0);
    c.setTextSize(2);
    c.setTextDatum(textdatum_t::middle_center);
    c.setTextColor(PRIMARY);
    c.drawString(buf, RING_X, RING_Y);

    // Panel derecho
    c.setTextDatum(textdatum_t::top_left);
    c.setTextSize(2);
    c.setTextColor(phaseCol);
    c.drawString(tr(work ? Str::PomoWork : Str::PomoBreak), PANEL_X, 26);

    c.setTextSize(1);
    c.setTextColor(GRAY);
    switch (run) {
        case Run::Ready:    c.drawString(tr(Str::PomoReady), PANEL_X, 48); break;
        case Run::Running:  c.drawString(tr(Str::PomoRunning), PANEL_X, 48); break;
        case Run::Paused:   c.drawString(tr(Str::PomoPaused), PANEL_X, 48); break;
        case Run::Finished:
            c.setTextColor(phaseCol);
            c.drawString(tr(work ? Str::PomoFinWork : Str::PomoFinBreak), PANEL_X, 48);
            break;
    }

    char line[32];
    snprintf(line, sizeof(line), tr(Str::PomoTodayFmt), pomodoroService.todayCount());
    c.setTextColor(STEM);
    c.drawString(line, PANEL_X, 64);

    c.setTextColor(DARKGRAY);
    if (run == Run::Ready && work) {
        snprintf(line, sizeof(line), tr(Str::PomoDurationFmt), pomodoroService.workMin());
        c.drawString(line, PANEL_X, 84);
    }
    switch (run) {
        case Run::Ready:    c.drawString(tr(Str::PomoStartHint), PANEL_X, 96); break;
        case Run::Running:  c.drawString(tr(Str::PomoPauseHint), PANEL_X, 96); break;
        case Run::Paused:   c.drawString(tr(Str::PomoResumeHint), PANEL_X, 96); break;
        case Run::Finished: c.drawString(tr(Str::PomoNextHint), PANEL_X, 96); break;
    }
    c.drawString(tr(Str::PomoResetHint), PANEL_X, 108);
}

void PomodoroApp::onKey(const KeyEvent& e) {
    switch (e.key) {
        case Key::Ok:
            pomodoroService.primaryAction();
            break;
        case Key::Left:
            pomodoroService.adjustWork(-5);
            break;
        case Key::Right:
            pomodoroService.adjustWork(+5);
            break;
        case Key::Char:
            if (e.ch == 'r') pomodoroService.reset();
            break;
        case Key::Back:
            appManager.goBack();
            break;
        default:
            break;
    }
    // El servicio sube revision() con cada cambio; update() repintará.
}
