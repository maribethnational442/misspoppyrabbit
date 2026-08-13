#pragma once
#include "Models.h"

// Persistencia de tareas en /mspos/tasks.json.
// Separado de la app: en v0.3 la WebUI leerá/escribirá tareas por aquí también.
namespace taskrepo {

// Devuelve el nº de tareas leídas; 0 si el archivo aún no existe;
// -1 sin microSD; -2 si el JSON está corrupto (no se pierde: no se sobreescribe
// hasta el próximo guardado explícito).
int load(models::Task* out, int maxTasks);

bool save(const models::Task* tasks, int count);

} // namespace taskrepo
