#pragma once
#include <M5GFX.h>

// Barra de estado superior, siempre visible: hora | WiFi + batería.
// No es una App: la pinta el AppManager encima de cualquier app.
class StatusBar {
public:
    void draw(M5Canvas& c);

private:
    void drawWifi(M5Canvas& c, int x, int y);
    void drawBattery(M5Canvas& c, int x, int y);
};
