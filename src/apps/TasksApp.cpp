#include "TasksApp.h"
#include "../core/AppManager.h"
#include "../core/StorageService.h"
#include "../models/TaskRepo.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/Icons.h"
#include "../ui/assets/PoppySprites.h"
#include <cstring>

namespace {
constexpr int LIST_Y = 20;
constexpr int ROW_H  = 13;
constexpr uint32_t SAVE_DEBOUNCE_MS = 1500;
}

const char* const* TasksApp::icon() const { return icons::TASKS; }

void TasksApp::onEnter() {
    _mode = Mode::List;
    if (!_loaded) {
        const int n = taskrepo::load(_tasks, MAX_TASKS);
        _corrupt = (n == -2);
        _count = (n > 0) ? n : 0;
        // id siguiente = mayor id existente + 1 (ids estables entre sesiones)
        for (int i = 0; i < _count; ++i) {
            if (_tasks[i].id >= _nextId) _nextId = _tasks[i].id + 1;
        }
        _loaded = true;
        _nav.visible = 7;
    }
    rebuildOrder();
}

void TasksApp::onExit() {
    // No dejamos un guardado pendiente en el aire al salir de la app
    if (_dirtySave) saveNow();
}

void TasksApp::update(uint32_t dtMs) {
    (void)dtMs;
    // Debounce de guardado: marcar 3 tareas seguidas = 1 escritura a SD,
    // no 3. La SD es lenta (~decenas de ms) y no queremos escrituras inútiles.
    if (_dirtySave && millis() >= _saveAt) saveNow();
}

void TasksApp::markDirty() {
    _dirtySave = true;
    _saveAt = millis() + SAVE_DEBOUNCE_MS;
}

void TasksApp::saveNow() {
    _dirtySave = false;
    if (!taskrepo::save(_tasks, _count)) {
        log_w("No se pudo guardar tasks.json");
    }
}

// Presentación: pendientes primero (en su orden), hechas al final.
void TasksApp::rebuildOrder() {
    int n = 0;
    for (int i = 0; i < _count; ++i) {
        if (!(_tasks[i].flags & models::EVT_DONE)) _order[n++] = i;
    }
    for (int i = 0; i < _count; ++i) {
        if (_tasks[i].flags & models::EVT_DONE) _order[n++] = i;
    }
    _nav.clampTo(_count);
}

models::Task* TasksApp::selectedTask() {
    if (_count == 0) return nullptr;
    return &_tasks[_order[_nav.sel]];
}

void TasksApp::addTask(const char* title) {
    if (_count >= MAX_TASKS || title[0] == '\0') return;
    models::Task& t = _tasks[_count++];
    t.id = _nextId++;
    strncpy(t.title, title, sizeof(t.title) - 1);
    t.title[sizeof(t.title) - 1] = '\0';
    t.due = 0;
    t.calendarId = 0;
    t.priority = 1;
    t.flags = 0;
    rebuildOrder();
    markDirty();
}

void TasksApp::removeSelected() {
    if (_count == 0) return;
    const int idx = _order[_nav.sel];
    // Compactar el array: mover una posición todo lo que está detrás
    for (int i = idx; i < _count - 1; ++i) _tasks[i] = _tasks[i + 1];
    --_count;
    rebuildOrder();
    markDirty();
}

void TasksApp::draw(M5Canvas& c) {
    drawList(c);
    if (_mode == Mode::NewTask) drawNewTask(c);
    else if (_mode == Mode::ConfirmDelete) drawConfirmDelete(c);
}

void TasksApp::drawList(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    if (!storage.mounted()) {
        c.setTextColor(POPPY);
        c.drawString("Sin microSD: los cambios no se guardan", PADDING, LIST_Y - 2);
    } else if (_corrupt) {
        c.setTextColor(POPPY);
        c.drawString("tasks.json ilegible: se sobreescribira", PADDING, LIST_Y - 2);
    }

    if (_count == 0) {
        // Estado vacío con la mascota
        pixelart::draw(c, sprites::RABBIT_IDLE, sprites::RABBIT_H, 104, 40, 2);
        c.setTextDatum(textdatum_t::top_center);
        c.setTextColor(GRAY);
        c.drawString("Sin tareas. Pulsa [n] para crear una.", SCREEN_W / 2, 82);
    }

    const int headerOffset = (!storage.mounted() || _corrupt) ? 10 : 0;
    for (int v = 0; v < _nav.visible; ++v) {
        const int row = _nav.scroll + v;
        if (row >= _count) break;
        const models::Task& t = _tasks[_order[row]];
        const int y = LIST_Y + headerOffset + v * ROW_H;
        const bool selected = (row == _nav.sel);
        const bool done = t.flags & models::EVT_DONE;

        if (selected) c.fillRoundRect(2, y - 2, SCREEN_W - 4, ROW_H, 3, POPPY_DIM);

        // Checkbox: hueco = pendiente, verde con marca = hecha
        if (done) {
            c.fillRect(PADDING + 2, y + 1, 8, 8, STEM);
            c.drawLine(PADDING + 4, y + 5, PADDING + 6, y + 7, BG);
            c.drawLine(PADDING + 6, y + 7, PADDING + 9, y + 2, BG);
        } else {
            c.drawRect(PADDING + 2, y + 1, 8, 8, PRIMARY);
        }

        // Color del título por prioridad; hechas en gris y tachadas
        uint16_t col = (t.priority == 2) ? POPPY : (t.priority == 0) ? GRAY : PRIMARY;
        if (done) col = DARKGRAY;
        c.setTextColor(selected && !done ? PRIMARY : col);
        c.setTextDatum(textdatum_t::top_left);
        c.drawString(t.title, PADDING + 16, y);
        if (done) {
            c.drawFastHLine(PADDING + 16, y + 4, c.textWidth(t.title), GRAY);
        }
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString("n:nueva d:borrar p:prio ENTER:hecha", PADDING, SCREEN_H - 2);
}

void TasksApp::drawNewTask(M5Canvas& c) {
    using namespace theme;
    // Panel modal sobre la lista
    c.fillRoundRect(10, 38, SCREEN_W - 20, 60, 5, BG);
    c.drawRoundRect(10, 38, SCREEN_W - 20, 60, 5, POPPY);
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);
    c.setTextColor(PRIMARY);
    c.drawString("Nueva tarea:", 18, 46);
    _input.draw(c, 18, 58, SCREEN_W - 36);
    c.setTextColor(DARKGRAY);
    c.drawString("[ENTER] crear   [`] cancelar", 18, 84);
}

void TasksApp::drawConfirmDelete(M5Canvas& c) {
    using namespace theme;
    const models::Task* t = selectedTask();
    if (t == nullptr) return;
    c.fillRoundRect(10, 45, SCREEN_W - 20, 46, 5, BG);
    c.drawRoundRect(10, 45, SCREEN_W - 20, 46, 5, POPPY);
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);
    c.setTextColor(PRIMARY);
    char msg[64];
    snprintf(msg, sizeof(msg), "Borrar '%.30s'?", t->title);
    c.drawString(msg, 18, 53);
    c.setTextColor(DARKGRAY);
    c.drawString("[ENTER] si   [`] no", 18, 73);
}

void TasksApp::onKey(const KeyEvent& e) {
    if (_mode == Mode::NewTask) {
        if (_input.handleKey(e)) {
            requestRedraw();
            return;
        }
        if (e.key == Key::Ok) {
            addTask(_input.buf);
            _mode = Mode::List;
            requestRedraw();
        } else if (e.key == Key::Back) {
            _mode = Mode::List;
            requestRedraw();
        }
        return;
    }

    if (_mode == Mode::ConfirmDelete) {
        if (e.key == Key::Ok) {
            removeSelected();
            _mode = Mode::List;
            requestRedraw();
        } else if (e.key == Key::Back) {
            _mode = Mode::List;
            requestRedraw();
        }
        return;
    }

    // Modo lista
    if (_nav.onKey(e, _count)) {
        requestRedraw();
        return;
    }
    switch (e.key) {
        case Key::Ok:
            if (models::Task* t = selectedTask()) {
                t->flags ^= models::EVT_DONE;   // alternar hecho/pendiente
                rebuildOrder();
                markDirty();
                requestRedraw();
            }
            break;
        case Key::Char:
            if (e.ch == 'n' && _count < MAX_TASKS) {
                _input.clear();
                _input.maxLen = (int)sizeof(models::Task{}.title) - 1;
                _mode = Mode::NewTask;
                requestRedraw();
            } else if (e.ch == 'd' && _count > 0) {
                _mode = Mode::ConfirmDelete;
                requestRedraw();
            } else if (e.ch == 'p') {
                if (models::Task* t = selectedTask()) {
                    t->priority = (t->priority + 1) % 3;   // baja→media→alta
                    markDirty();
                    requestRedraw();
                }
            }
            break;
        case Key::Back:
            appManager.goBack();
            break;
        default:
            break;
    }
}
