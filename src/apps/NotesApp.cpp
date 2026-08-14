#include "NotesApp.h"
#include <ctime>
#include "../core/AppManager.h"
#include "../core/StorageService.h"
#include "../ui/Theme.h"
#include "../ui/PixelArt.h"
#include "../ui/assets/Icons.h"
#include "../ui/assets/PoppySprites.h"
#include "RecorderApp.h"

NotesApp notesApp;

namespace {
constexpr int LIST_Y = 18;
constexpr int ROW_H  = 14;

// "14 Aug 12:30" desde el id (epoch); ids pre-NTP salen como #crudo
void fmtWhen(uint32_t id, char* out, size_t cap) {
    if (id < 1600000000) {
        snprintf(out, cap, "#%u", (unsigned)id);
        return;
    }
    time_t t = (time_t)id;
    struct tm tv;
    localtime_r(&t, &tv);
    snprintf(out, cap, "%02d %s %02d:%02d", tv.tm_mday, lang::months()[tv.tm_mon],
             tv.tm_hour, tv.tm_min);
}
}

const char* const* NotesApp::icon() const { return icons::NOTES; }

void NotesApp::onEnter() {
    _mode = Mode::List;
    _nav.visible = 7;
    _lastRev = notesService.revision();
    _nav.clampTo(notesService.count());
}

void NotesApp::update(uint32_t dtMs) {
    (void)dtMs;
    if (notesService.revision() != _lastRev) {
        _lastRev = notesService.revision();
        _nav.clampTo(notesService.count());
        requestRedraw();
    }
}

const NotesService::Meta* NotesApp::selected() {
    if (notesService.count() == 0) return nullptr;
    return &notesService.notes()[_nav.sel];
}

void NotesApp::draw(M5Canvas& c) {
    switch (_mode) {
        case Mode::List:          drawList(c); break;
        case Mode::Editor:        drawEditor(c); break;
        case Mode::ConfirmDelete: drawList(c); drawConfirmDelete(c); break;
    }
}

void NotesApp::drawList(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    const int count = notesService.count();
    if (count == 0) {
        pixelart::draw(c, sprites::RABBIT_IDLE, sprites::RABBIT_H, 104, 38, 2);
        c.setTextDatum(textdatum_t::top_center);
        c.setTextColor(GRAY);
        c.drawString(tr(Str::NotesEmpty), SCREEN_W / 2, 80);
    }

    for (int v = 0; v < _nav.visible; ++v) {
        const int row = _nav.scroll + v;
        if (row >= count) break;
        const NotesService::Meta& m = notesService.notes()[row];
        const int y = LIST_Y + v * ROW_H;
        const bool sel = (row == _nav.sel);
        if (sel) c.fillRoundRect(2, y - 1, SCREEN_W - 4, ROW_H, 3, POPPY_DIM);

        pixelart::draw(c, m.type == NotesService::Type::Text ? icons::MINI_TXT
                                                             : icons::MINI_MIC,
                       icons::MINI_H, PADDING + 1, y + 2, 1);

        char when[20];
        fmtWhen(m.id, when, sizeof(when));
        c.setTextColor(sel ? PRIMARY : GRAY);
        c.drawString(when, PADDING + 14, y + 3);

        char right[32];
        if (m.type == NotesService::Type::Text) {
            snprintf(right, sizeof(right), "%.20s", m.preview);
            c.setTextColor(sel ? PRIMARY : GRAY);
        } else {
            const int d = notesService.durationS(m);
            const bool now = notesService.playing() && notesService.playingId() == m.id;
            snprintf(right, sizeof(right), "%s %02d:%02d", now ? "\x10" : " ", d / 60, d % 60);
            c.setTextColor(now ? STEM : (sel ? PRIMARY : GRAY));
        }
        c.drawString(right, 116, y + 3);
    }

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::NotesHint), PADDING, SCREEN_H - 2);
}

void NotesApp::drawEditor(M5Canvas& c) {
    using namespace theme;
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);

    // Word-wrap simple a 38 columnas, mostrando las últimas líneas que quepan
    constexpr int COLS = 38, LINES = 11;
    static char lines[LINES][COLS + 1];
    int nLines = 0, col = 0;
    lines[0][0] = '\0';
    for (size_t i = 0; i < _len; ++i) {
        if (col >= COLS || _text[i] == '\n') {
            if (nLines < LINES - 1) ++nLines;
            else {   // scroll: descartar la primera línea
                for (int l = 0; l < LINES - 1; ++l) memcpy(lines[l], lines[l + 1], COLS + 1);
            }
            col = 0;
            lines[nLines][0] = '\0';
            if (_text[i] == '\n') continue;
        }
        lines[nLines][col++] = _text[i];
        lines[nLines][col] = '\0';
    }

    c.setTextColor(PRIMARY);
    for (int l = 0; l <= nLines; ++l) {
        c.drawString(lines[l], PADDING, 18 + l * 9);
    }
    // Cursor de bloque al final
    c.fillRect(PADDING + col * 6 + 1, 18 + nLines * 9, 5, 8, POPPY);

    c.setTextDatum(textdatum_t::bottom_left);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::EditorHint), PADDING, SCREEN_H - 2);
}

void NotesApp::drawConfirmDelete(M5Canvas& c) {
    using namespace theme;
    const NotesService::Meta* m = selected();
    if (m == nullptr) return;
    c.fillRoundRect(10, 45, SCREEN_W - 20, 46, 5, BG);
    c.drawRoundRect(10, 45, SCREEN_W - 20, 46, 5, POPPY);
    c.setFont(&fonts::Font0);
    c.setTextSize(1);
    c.setTextDatum(textdatum_t::top_left);
    c.setTextColor(PRIMARY);
    char msg[64];
    snprintf(msg, sizeof(msg), tr(Str::DeleteConfirmFmt),
             (m->type == NotesService::Type::Text && m->preview[0]) ? m->preview
                                                                    : tr(Str::VoiceLabel));
    c.drawString(msg, 18, 53);
    c.setTextColor(DARKGRAY);
    c.drawString(tr(Str::DeleteHint), 18, 73);
}

void NotesApp::onKey(const KeyEvent& e) {
    if (_mode == Mode::Editor) {
        if (e.key == Key::Char && e.ch >= 32 && _len < sizeof(_text) - 1) {
            _text[_len++] = e.ch;
            _text[_len] = '\0';
            requestRedraw();
        } else if (e.key == Key::Backspace && _len > 0) {
            _text[--_len] = '\0';
            requestRedraw();
        } else if (e.key == Key::Ok) {
            if (_len > 0) notesService.saveText(_editId, _text);
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
            if (const NotesService::Meta* m = selected()) notesService.removeNote(m->id);
            _mode = Mode::List;
            requestRedraw();
        } else if (e.key == Key::Back) {
            _mode = Mode::List;
            requestRedraw();
        }
        return;
    }

    // Lista
    if (_nav.onKey(e, notesService.count())) {
        requestRedraw();
        return;
    }
    switch (e.key) {
        case Key::Ok: {
            const NotesService::Meta* m = selected();
            if (m == nullptr) break;
            if (m->type == NotesService::Type::Text) {
                const int n = notesService.readText(m->id, _text, sizeof(_text));
                if (n >= 0) {
                    _len = (size_t)n;
                    _editId = m->id;
                    _mode = Mode::Editor;
                    requestRedraw();
                }
            } else {
                if (notesService.playing() && notesService.playingId() == m->id) {
                    notesService.playStop();
                } else {
                    notesService.playStart(m->id);
                }
                requestRedraw();
            }
            break;
        }
        case Key::Char:
            if (e.ch == 'n') {
                _text[0] = '\0';
                _len = 0;
                _editId = 0;
                _mode = Mode::Editor;
                requestRedraw();
            } else if (e.ch == 'm') {
                appManager.launch(&recorderApp);
            } else if (e.ch == 'd' && notesService.count() > 0) {
                _mode = Mode::ConfirmDelete;
                requestRedraw();
            }
            break;
        case Key::Back:
            notesService.playStop();
            appManager.goBack();
            break;
        default:
            break;
    }
}
