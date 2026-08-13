# Ms P OS — Arquitectura

Firmware tipo launcher + apps para M5Stack Cardputer ADV (ESP32-S3, 512KB
SRAM **sin PSRAM**, 8MB flash). Identidad: conejo blanco con poppy roja,
paleta negro/blanco/poppy/verde definida en `src/ui/Theme.h`.

## Roadmap

- **v0.1 (actual)**: núcleo — framework de apps, launcher, Settings con WiFi
- v0.2: Tareas + Pomodoro con persistencia en microSD
- v0.3: WebUI desde el dispositivo (ESPAsyncWebServer + WebSocket)
- v0.4: Agenda multi-calendario (4 contextos con color, alertas sonoras)
- v0.5: `POST /api/events/import` (JSON desde extensión de Chrome)

## Capas

```
main.cpp            registra apps, arranca servicios; loop() solo delega
├── core/           el "OS": no conoce apps concretas
│   ├── App.h           contrato: onEnter/update/draw/onKey/onExit
│   ├── AppManager      pila de apps + registro + canvas único + teclado
│   ├── StatusBar       hora | WiFi | batería (siempre visible)
│   ├── WifiService     FSM no bloqueante, credenciales NVS, NTP, escaneo async
│   └── Config.h        constantes (zona horaria, hostname, versión)
├── ui/             identidad visual
│   ├── Theme.h         ÚNICO punto de verdad de paleta y métricas
│   ├── PixelArt.h      render de sprites "mapa de caracteres"
│   ├── BootScreen.h    arranque animado
│   └── assets/         mascota (PoppySprites.h) e iconos (Icons.h)
├── apps/           Launcher, Settings, Reloj, About
└── models/         Models.h: Task/Event/Calendar (esquema del roadmap completo)
```

## Decisiones clave (y por qué)

- **Pila de apps** en vez de puntero único: "volver atrás" multinivel gratis.
  Solo la app del tope recibe update/draw/teclas.
- **Un solo M5Canvas** de 240×135×16bit (~63KB) compartido, creado una vez.
  Volcado completo con `pushSprite` → sin parpadeo. Si la RAM aprieta en
  v0.3, plan B: bajar a 8 bits con paleta (~32KB).
- **Todo estático, nada de new/delete al navegar**: sin PSRAM, la
  fragmentación de heap es el enemigo. Apps globales, buffers de tamaño fijo
  (`char[N]`, no `String` en estructuras persistentes).
- **draw() solo con dirty flag**: update() corre a ~30fps, pero solo se
  repinta cuando algo cambió (~10ms de SPI por frame que nos ahorramos).
  La StatusBar fuerza un repintado por segundo (reloj).
- **WiFi como servicio, no como app**: reconexión al arrancar y estado en la
  StatusBar no pueden depender de que Settings esté abierta. Escaneo siempre
  asíncrono (el bloqueante congela la UI 2-3s).
- **Teclado semántico**: AppManager traduce la matriz a `KeyEvent`
  (`; . , /` = flechas, ENTER=Ok, `` ` ``=Volver, DEL=Backspace). Las apps con
  entrada de texto activan `wantsTextInput()` para recibir esos símbolos
  como caracteres (contraseñas).
- **Sprites como arrays de strings** (1 carácter = 1 píxel): editables a mano
  sin herramientas, coste de flash trivial.
- **Models.h ya contempla v0.4/v0.5**: `Event` con `calendarId`, color por
  calendario, `alertMinBefore`, `id` estable + flag `EVT_SYNCED` para que el
  import repetido sea idempotente.

## Convenciones

- La lib M5Cardputer (no M5Unified a pelo): el ADV usa teclado TCA8418 por
  I2C y la lib lo abstrae.
- Zona horaria en `Config.h` (`config::TIMEZONE`), formato POSIX TZ.
- NVS namespace único: `mspos`.
