// ============================================================================
// Miss Poppy Rabbit — punto de entrada.
// setup() arranca hardware y servicios; loop() solo delega: el trabajo real
// vive en AppManager (UI) y WifiService (red). main.cpp es el único sitio
// que conoce la lista concreta de apps.
// ============================================================================
#include <M5Cardputer.h>

#include "core/AppManager.h"
#include "core/Lang.h"
#include "core/Sound.h"
#include "core/WifiService.h"
#include "core/StorageService.h"
#include "core/TaskStore.h"
#include "core/CalendarStore.h"
#include "core/PomodoroService.h"
#include "core/AlertService.h"
#include "core/WebService.h"
#include "ui/BootScreen.h"
#include "apps/LauncherApp.h"
#include "apps/AgendaApp.h"
#include "apps/TasksApp.h"
#include "apps/PomodoroApp.h"
#include "apps/SettingsApp.h"
#include "apps/ClockApp.h"
#include "apps/AboutApp.h"
#include "apps/AlertApp.h"

// Apps como objetos globales estáticos: viven toda la sesión, cero heap,
// cero fragmentación (clave sin PSRAM).
static LauncherApp launcherApp;
static TasksApp    tasksApp;
static PomodoroApp pomodoroApp;
static SettingsApp settingsApp;
static ClockApp    clockApp;
static AboutApp    aboutApp;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);   // true = inicializar también el teclado
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(80);

    appManager.begin();             // crea el canvas de 240×135

    lang::begin();                  // idioma guardado (defecto: ingles)
    sound::begin();                 // volumen guardado
    wifiService.begin();            // no bloquea: conecta durante el boot screen
    storage.begin();                // monta la microSD (si hay)
    taskStore.begin();              // carga las tareas desde la SD
    calendarStore.begin();          // carga agenda y calendarios
    webService.begin();             // rutas listas; arranca al conectar el WiFi
    showBootScreen(appManager.canvas());

    // Registro = lo que lista el Launcher (el propio Launcher no se registra)
    appManager.registerApp(&agendaApp);
    appManager.registerApp(&tasksApp);
    appManager.registerApp(&pomodoroApp);
    appManager.registerApp(&settingsApp);
    appManager.registerApp(&clockApp);
    appManager.registerApp(&aboutApp);
    appManager.launch(&launcherApp);
}

void loop() {
    wifiService.loop();
    taskStore.loop();        // consume comandos de la web + guardado debounced
    calendarStore.loop();    // idem para la agenda
    pomodoroService.loop();  // el temporizador corre aunque la app no esté abierta
    alertService.loop();     // vigila las reuniones que se acercan

    // Si hay una alerta activa, el banner se impone sobre lo que haya
    if (alertService.active() && appManager.topApp() != &alertApp) {
        appManager.launch(&alertApp);
    }

    webService.loop();       // broadcasts de WebSocket, arranque diferido
    appManager.tick();
}
