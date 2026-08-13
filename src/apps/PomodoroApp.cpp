#include "PomodoroApp.h"
#include "../core/AppManager.h"
#include "../core/Sound.h"
#include "../ui/Theme.h"
#include "../ui/assets/Icons.h"

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

uint32_t PomodoroApp::phaseTotalMs() const {
    return ((_phase == Phase::Work) ? _workMin : _breakMin) * 60000u;
}

uint32_t PomodoroApp::remainingMs() const {
    switch (_run) {
        case Run::Running: {
            const uint32_t now = millis();
            return (now >= _endAt) ? 0 : _endAt - now;
        }
        case Run::Paused:  return _pausedRemain;
        case Run::Ready:   return phaseTotalMs();
        default:           return 0;
    }
}

void PomodoroApp::startPhase() {
    _endAt = millis() + remainingMs();   // desde Ready arranca completo; desde Paused, lo restante
    _run = Run::Running;
    sound::click();
}

void PomodoroApp::finishPhase() {
    _run = Run::Finished;
    if (_phase == Phase::Work) {
        ++_todayCount;
        sound::workDone();
    } else {
        sound::breakDone();
    }
}

void PomodoroApp::update(uint32_t dtMs) {
    (void)dtMs;
    if (_run == Run::Running) {
        if (remainingMs() == 0) {
            finishPhase();
            requestRedraw();
            return;
        }
        // Repintar solo cuando cambia el segundo mostrado
        const int sec = (int)(remainingMs() / 1000);
        if (sec != _lastShownSec) {
            _lastShownSec = sec;
            requestRedraw();
        }
    }
}

void PomodoroApp::draw(M5Canvas& c) {
    using namespace theme;
    const bool work = (_phase == Phase::Work);
    const uint16_t phaseCol = work ? POPPY : STEM;

    // Anillo: se va VACIANDO con el tiempo (lleno = todo por delante)
    const float progress = (float)remainingMs() / (float)phaseTotalMs();
    drawProgressRing(c, progress, phaseCol);

    // Tiempo restante en el centro
    const uint32_t rem = remainingMs();
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
    c.drawString(work ? "TRABAJO" : "DESCANSO", PANEL_X, 26);

    c.setTextSize(1);
    c.setTextColor(GRAY);
    switch (_run) {
        case Run::Ready:    c.drawString("Listo para empezar", PANEL_X, 48); break;
        case Run::Running:  c.drawString("En marcha...", PANEL_X, 48); break;
        case Run::Paused:   c.drawString("En pausa", PANEL_X, 48); break;
        case Run::Finished:
            c.setTextColor(phaseCol);
            c.drawString(work ? "Termino! A descansar" : "Termino! A trabajar", PANEL_X, 48);
            break;
    }

    char line[32];
    snprintf(line, sizeof(line), "Hoy: %d pomodoros", _todayCount);
    c.setTextColor(STEM);
    c.drawString(line, PANEL_X, 64);

    c.setTextColor(DARKGRAY);
    if (_run == Run::Ready && work) {
        snprintf(line, sizeof(line), ", / duracion: %d min", _workMin);
        c.drawString(line, PANEL_X, 84);
    }
    switch (_run) {
        case Run::Ready:    c.drawString("ENTER: empezar", PANEL_X, 96); break;
        case Run::Running:  c.drawString("ENTER: pausar", PANEL_X, 96); break;
        case Run::Paused:   c.drawString("ENTER: seguir", PANEL_X, 96); break;
        case Run::Finished: c.drawString("ENTER: siguiente", PANEL_X, 96); break;
    }
    c.drawString("r: reiniciar", PANEL_X, 108);
}

void PomodoroApp::onKey(const KeyEvent& e) {
    switch (e.key) {
        case Key::Ok:
            switch (_run) {
                case Run::Ready:
                    startPhase();
                    break;
                case Run::Running:
                    _pausedRemain = remainingMs();
                    _run = Run::Paused;
                    sound::click();
                    break;
                case Run::Paused:
                    startPhase();   // reanuda con lo que quedaba
                    break;
                case Run::Finished:
                    // Pasar al siguiente tramo y arrancarlo directamente
                    _phase = (_phase == Phase::Work) ? Phase::Break : Phase::Work;
                    _run = Run::Ready;
                    startPhase();
                    break;
            }
            requestRedraw();
            break;

        case Key::Left:
            if (_run == Run::Ready && _phase == Phase::Work && _workMin > 5) {
                _workMin -= 5;
                requestRedraw();
            }
            break;
        case Key::Right:
            if (_run == Run::Ready && _phase == Phase::Work && _workMin < 60) {
                _workMin += 5;
                requestRedraw();
            }
            break;

        case Key::Char:
            if (e.ch == 'r') {
                _phase = Phase::Work;
                _run = Run::Ready;
                requestRedraw();
            }
            break;

        case Key::Back:
            appManager.goBack();   // el temporizador sigue: estado estático
            break;

        default:
            break;
    }
}
