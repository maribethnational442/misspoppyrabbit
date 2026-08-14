#pragma once
#include <cstdint>

// ============================================================================
// Lang.h — internacionalización de Miss Poppy Rabbit.
//
// Todas las cadenas visibles viven en UN catálogo: un enum de IDs y una tabla
// por idioma (en flash, cero RAM extra). Las apps piden tr(Str::Algo) y
// reciben la cadena del idioma activo. El idioma se guarda en NVS y por
// defecto es inglés. Ojo: la fuente pixel (Font0) es ASCII puro, así que las
// cadenas en español van sin tildes ni eñes a propósito.
// ============================================================================

enum class Str : uint8_t {
    // Nombres de apps (el launcher los lista)
    AppTasks, AppPomodoro, AppSettings, AppClock, AppAbout,
    // Launcher
    LauncherHint,
    // Settings / WiFi
    WifiConnectingFmt, WifiFailedFmt, WifiNone,
    ScanRow, ScanningRow, ForgetRowFmt, LanguageRowFmt,
    SettingsHint, PasswordTitleFmt, PasswordHint,
    // Tareas
    TasksNoSd, TasksCorrupt, TasksEmpty, TasksHint,
    NewTaskTitle, NewTaskHint, DeleteConfirmFmt, DeleteHint,
    // Pomodoro
    PomoWork, PomoBreak, PomoReady, PomoRunning, PomoPaused,
    PomoFinWork, PomoFinBreak, PomoTodayFmt, PomoDurationFmt,
    PomoStartHint, PomoPauseHint, PomoResumeHint, PomoNextHint, PomoResetHint,
    // Reloj
    ClockNoTime1, ClockNoTime2, ClockNoTime3,
    // About
    AboutRamFmt,
    // Agenda (v0.4)
    AppAgenda,
    AgendaEmptyDay, AgendaHintDay, AgendaHintWeek,
    QuickTitle, QuickWhenHint, QuickSaveHint, QuickToday, QuickTomorrow,
    AlertIn10, AlertIn2, AlertConfirm, AlertConfirmHint, AlertDismissHint,
    // Ajustes extra
    VolumeRowFmt,
    // Reminders
    ReminderHead, ReminderHint, QuickRemToggle,
    // Borrado de datos
    EraseRow, EraseConfirm1, EraseConfirm2,
    // Resumen del día y zona horaria
    BriefTitle, BriefEmpty, BriefConflictFmt, BriefRowFmt, TzRowFmt,
    COUNT
};

namespace lang {

enum class Code : uint8_t { EN = 0, ES = 1 };

void begin();               // carga el idioma guardado en NVS (defecto: EN)
void set(Code c);           // cambia y persiste
Code current();
const char* code();         // "en" / "es" — para la API y la WebUI
const char* displayName();  // "English" / "Espanol" — para el menu

const char* tr(Str s);
const char* const* days();     // 7 nombres cortos de dia (para el Reloj)
const char* const* months();   // 12 nombres cortos de mes

} // namespace lang

// Atajo global: las apps escriben tr(Str::TasksEmpty) a secas.
inline const char* tr(Str s) { return lang::tr(s); }
