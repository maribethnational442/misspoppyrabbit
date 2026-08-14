#pragma once
#include <ctime>
#include "../core/App.h"
#include "../core/Lang.h"
#include "../ui/widgets/TextField.h"

// Agenda multi-calendario: vista día, vista semana y alta rápida local.
// Los eventos viven en CalendarStore (la WebUI es el otro cliente) y las
// alertas las dispara AlertService — esta app solo muestra y crea.
class AgendaApp : public App {
public:
    const char* name() const override { return tr(Str::AppAgenda); }
    const char* const* icon() const override;

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool wantsTextInput() const override { return _mode == Mode::QuickTitle; }

private:
    enum class Mode : uint8_t { Day, Week, QuickTitle, QuickWhen };

    void drawDay(M5Canvas& c);
    void drawWeek(M5Canvas& c);
    void drawQuickTitle(M5Canvas& c);
    void drawQuickWhen(M5Canvas& c);
    void saveQuickEvent();

    static time_t startOfDay(time_t t);

    Mode   _mode = Mode::Day;
    time_t _day = 0;              // 00:00 local del día mostrado/seleccionado
    uint32_t _lastRev = 0;

    // Alta rápida
    TextField _title;
    int _qDayOffset = 0;          // 0 = hoy, 1 = mañana
    int _qMinOfDay = 9 * 60;      // minutos desde medianoche
    int _qCal = 0;
};

extern AgendaApp agendaApp;
