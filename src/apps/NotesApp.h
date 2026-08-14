#pragma once
#include "../core/App.h"
#include "../core/Lang.h"
#include "../core/NotesService.h"
#include "../ui/widgets/ListNav.h"

// Lista cronológica de notas de texto y de voz. [n] texto, [m] voz,
// ENTER abre (texto → editor / voz → play-stop), [d] borra.
// El editor es deliberadamente simple (v0.7): escribes y borras desde el
// final, con word-wrap — el apunte rápido; la edición cómoda vive en la WebUI.
class NotesApp : public App {
public:
    const char* name() const override { return tr(Str::AppNotes); }
    const char* const* icon() const override;

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool wantsTextInput() const override { return _mode == Mode::Editor; }

private:
    enum class Mode : uint8_t { List, Editor, ConfirmDelete };

    void drawList(M5Canvas& c);
    void drawEditor(M5Canvas& c);
    void drawConfirmDelete(M5Canvas& c);
    const NotesService::Meta* selected();

    Mode     _mode = Mode::List;
    ListNav  _nav;
    uint32_t _lastRev = 0;

    // Editor (una sola nota en RAM a la vez)
    char     _text[NotesService::MAX_TEXT];
    size_t   _len = 0;
    uint32_t _editId = 0;
};

extern NotesApp notesApp;
