#pragma once
#include "../core/App.h"
#include "../models/Models.h"
#include "../ui/widgets/ListNav.h"
#include "../ui/widgets/TextField.h"

// Lista de tareas con persistencia en microSD.
// Teclas: [n] nueva  [d] borrar  [p] prioridad  [ENTER] hecha/pendiente
class TasksApp : public App {
public:
    const char* name() const override { return "Tareas"; }
    const char* const* icon() const override;

    void onEnter() override;
    void onExit() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool wantsTextInput() const override { return _mode == Mode::NewTask; }

private:
    enum class Mode : uint8_t { List, NewTask, ConfirmDelete };
    static constexpr int MAX_TASKS = 64;

    void rebuildOrder();          // pendientes arriba, hechas abajo
    void markDirty();             // programa un guardado con debounce
    void saveNow();
    void addTask(const char* title);
    void removeSelected();
    models::Task* selectedTask();

    void drawList(M5Canvas& c);
    void drawNewTask(M5Canvas& c);
    void drawConfirmDelete(M5Canvas& c);

    Mode _mode = Mode::List;
    models::Task _tasks[MAX_TASKS];
    int _count = 0;
    int _order[MAX_TASKS];        // índices en orden de presentación

    ListNav   _nav;
    TextField _input;

    uint32_t _nextId = 1;
    bool     _loaded = false;     // las tareas se cargan una sola vez
    bool     _corrupt = false;    // tasks.json ilegible
    bool     _dirtySave = false;
    uint32_t _saveAt = 0;
};
