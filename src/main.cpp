// ============================================================================
// Ms P OS — punto de entrada.
// setup() arranca hardware y servicios; loop() solo delega: el trabajo real
// vive en AppManager (UI) y WifiService (red). main.cpp es el único sitio
// que conoce la lista concreta de apps.
// ============================================================================
#include <M5Cardputer.h>

#include "core/AppManager.h"
#include "core/WifiService.h"
#include "ui/BootScreen.h"
#include "apps/LauncherApp.h"
#include "apps/SettingsApp.h"
#include "apps/ClockApp.h"
#include "apps/AboutApp.h"

// Apps como objetos globales estáticos: viven toda la sesión, cero heap,
// cero fragmentación (clave sin PSRAM).
static LauncherApp launcherApp;
static SettingsApp settingsApp;
static ClockApp    clockApp;
static AboutApp    aboutApp;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);   // true = inicializar también el teclado
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(80);

    appManager.begin();             // crea el canvas de 240×135

    wifiService.begin();            // no bloquea: conecta durante el boot screen
    showBootScreen(appManager.canvas());

    // Registro = lo que lista el Launcher (el propio Launcher no se registra)
    appManager.registerApp(&settingsApp);
    appManager.registerApp(&clockApp);
    appManager.registerApp(&aboutApp);
    appManager.launch(&launcherApp);
}

void loop() {
    wifiService.loop();
    appManager.tick();
}
