#pragma once
#include "../core/App.h"
#include "../models/Models.h"
#include "../ui/widgets/ListNav.h"
#include "../ui/widgets/TextField.h"

// Cara en pantalla del TaskStore (los datos y su persistencia viven allí;
// la WebUI es el otro cliente del mismo servicio).
// Teclas: [n] nueva  [d] borrar  [p] prioridad  [ENTER] hecha/pendiente
class TasksApp : public App {
public:
    const char* name() const override { return "Tareas"; }
    const char* const* icon() const override;

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool wantsTextInput() const override { return _mode == Mode::NewTask; }

private:
    enum class Mode : uint8_t { List, NewTask, ConfirmDelete };

    void rebuildOrder();          // pendientes arriba, hechas abajo
    const models::Task* selectedTask();

    void drawList(M5Canvas& c);
    void drawNewTask(M5Canvas& c);
    void drawConfirmDelete(M5Canvas& c);

    Mode _mode = Mode::List;
    int  _order[64];              // índices del store en orden de presentación
    uint32_t _lastRev = 0;        // última revision() del store que pintamos

    ListNav   _nav;
    TextField _input;
};
