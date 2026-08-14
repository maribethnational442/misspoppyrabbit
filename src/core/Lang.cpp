#include "Lang.h"
#include <Preferences.h>
#include "Config.h"

namespace {

constexpr int N = (int)Str::COUNT;

// Las dos tablas van indexadas por el enum: si añades una cadena, añádela en
// AMBAS y en la MISMA posición (el compilador no avisa si una se queda corta,
// así que el static_assert de abajo cuenta los elementos).
const char* const EN[N] = {
    // Apps
    "Tasks", "Pomodoro", "Settings", "Clock", "About",
    // Launcher
    "[;] [.] move   [ENTER] open",
    // Settings / WiFi
    "WiFi: connecting to %s...", "WiFi: failed on %s", "WiFi: not connected",
    "> Scan networks", "Scanning...", "> Forget '%s'", "> Language: %s",
    "[ENTER] select  [`] back  * = locked",
    "Password for '%s':",
    "[ENTER] connect  [`] cancel  [DEL] erase",
    // Tasks
    "No microSD: changes won't be saved",
    "tasks.json unreadable: will be overwritten",
    "No tasks. Press [n] to add one.",
    "n:new d:delete p:prio ENTER:done",
    "New task:", "[ENTER] create   [`] cancel",
    "Delete '%.30s'?", "[ENTER] yes   [`] no",
    // Pomodoro
    "WORK", "BREAK", "Ready to start", "Running...", "Paused",
    "Done! Take a break", "Done! Back to work",
    "Today: %d pomodoros", ", / length: %d min",
    "ENTER: start", "ENTER: pause", "ENTER: resume", "ENTER: next", "r: reset",
    // Clock
    "No time yet.", "Connect WiFi in Settings", "to sync the clock (NTP).",
    // About
    "Free RAM: %u KB",
    // Agenda
    "Agenda",
    "No events this day.",
    "n:new v:week ,/:day t:today",
    "ENTER: open day   v: day view",
    "New event:",
    "; . time   , / day   c: calendar",
    "[ENTER] save   [`] cancel",
    "Today", "Tomorrow",
    "In 10 minutes:",
    "In 2 minutes!",
    "Are you in this meeting?",
    "[ENTER] yes, I'm in   [`] ask in 5 min",
    "any key to dismiss",
    // Ajustes extra
    "> Volume: %d%% (, / adjust)",
    // Reminders
    "Reminder:",
    "[ENTER] done   [`] snooze 5 min",
    "r: reminder",
};

const char* const ES[N] = {
    // Apps
    "Tareas", "Pomodoro", "Settings", "Reloj", "Acerca de",
    // Launcher
    "[;] [.] mover   [ENTER] abrir",
    // Settings / WiFi
    "WiFi: conectando a %s...", "WiFi: fallo con %s", "WiFi: sin conexion",
    "> Buscar redes", "Buscando redes...", "> Olvidar '%s'", "> Idioma: %s",
    "[ENTER] elegir  [`] volver  * = con clave",
    "Clave de '%s':",
    "[ENTER] conectar  [`] cancelar  [DEL] borrar",
    // Tareas
    "Sin microSD: los cambios no se guardan",
    "tasks.json ilegible: se sobreescribira",
    "Sin tareas. Pulsa [n] para crear una.",
    "n:nueva d:borrar p:prio ENTER:hecha",
    "Nueva tarea:", "[ENTER] crear   [`] cancelar",
    "Borrar '%.30s'?", "[ENTER] si   [`] no",
    // Pomodoro
    "TRABAJO", "DESCANSO", "Listo para empezar", "En marcha...", "En pausa",
    "Termino! A descansar", "Termino! A trabajar",
    "Hoy: %d pomodoros", ", / duracion: %d min",
    "ENTER: empezar", "ENTER: pausar", "ENTER: seguir", "ENTER: siguiente", "r: reiniciar",
    // Reloj
    "Sin hora todavia.", "Conecta WiFi en Settings", "para sincronizar (NTP).",
    // About
    "RAM libre: %u KB",
    // Agenda
    "Agenda",
    "Sin eventos este dia.",
    "n:nuevo v:semana ,/:dia t:hoy",
    "ENTER: abrir dia   v: vista dia",
    "Nuevo evento:",
    "; . hora   , / dia   c: calendario",
    "[ENTER] guardar   [`] cancelar",
    "Hoy", "Manana",
    "En 10 minutos:",
    "En 2 minutos!",
    "Estas ya en la reunion?",
    "[ENTER] si, estoy   [`] en 5 min",
    "cualquier tecla para cerrar",
    // Ajustes extra
    "> Volumen: %d%% (, / ajustar)",
    // Reminders
    "Recordatorio:",
    "[ENTER] hecho   [`] posponer 5 min",
    "r: recordatorio",
};

const char* const DAYS_EN[7]    = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const DAYS_ES[7]    = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};
const char* const MONTHS_EN[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const MONTHS_ES[12] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun",
                                   "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};

lang::Code g_code = lang::Code::EN;

constexpr const char* NVS_KEY = "lang";

} // namespace

namespace lang {

void begin() {
    Preferences prefs;
    prefs.begin(config::NVS_NS, /*readOnly=*/true);
    g_code = (prefs.getUChar(NVS_KEY, 0) == 1) ? Code::ES : Code::EN;
    prefs.end();
}

void set(Code c) {
    g_code = c;
    Preferences prefs;
    prefs.begin(config::NVS_NS, false);
    prefs.putUChar(NVS_KEY, (uint8_t)c);
    prefs.end();
}

Code current()            { return g_code; }
const char* code()        { return (g_code == Code::ES) ? "es" : "en"; }
const char* displayName() { return (g_code == Code::ES) ? "Espanol" : "English"; }

const char* tr(Str s) {
    const int i = (int)s;
    return (g_code == Code::ES) ? ES[i] : EN[i];
}

const char* const* days()   { return (g_code == Code::ES) ? DAYS_ES : DAYS_EN; }
const char* const* months() { return (g_code == Code::ES) ? MONTHS_ES : MONTHS_EN; }

} // namespace lang
