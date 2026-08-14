#pragma once
#include "../core/App.h"
#include "../core/WifiService.h"
#include "../ui/widgets/ListNav.h"
#include "../ui/widgets/TextField.h"
#include "../ui/widgets/Marquee.h"

// Cara visible del WifiService: estado, escaneo de redes, conexión con
// contraseña y "olvidar red". Dos modos internos: lista y entrada de texto.
class SettingsApp : public App {
public:
    const char* name() const override { return "Settings"; }
    const char* const* icon() const override;

    void onEnter() override;
    void update(uint32_t dtMs) override;
    void draw(M5Canvas& c) override;
    void onKey(const KeyEvent& e) override;
    bool wantsTextInput() const override { return _mode == Mode::Password; }

private:
    enum class Mode : uint8_t { List, Password };

    struct Net {
        char ssid[33];
        int  rssi;
        bool open;   // red sin contraseña
    };
    static constexpr int MAX_NETS = 10;

    int  rowCount() const;
    void activateRow(int row);
    void drawList(M5Canvas& c);
    void drawPassword(M5Canvas& c);

    Mode _mode = Mode::List;
    ListNav   _nav;
    TextField _pass;
    Marquee   _headerScroll;   // la cabecera "WiFi: red (ip)" puede no caber

    Net _nets[MAX_NETS];
    int _netCount = 0;
    int _targetNet = -1;

    WifiService::State _lastState = WifiService::State::Idle;
};
