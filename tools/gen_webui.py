# ============================================================================
# gen_webui.py — script pre-build de PlatformIO: comprime web/index.html y lo
# convierte en un header C (src/generated/webui_gz.h) que el firmware sirve
# directo desde flash. Así la WebUI viaja DENTRO del binario: un solo .bin
# para todos los canales de distribución (Web Installer, M5Burner,
# M5Launcher) y sin paso uploadfs.
#
# Corre solo en cada build via platformio.ini: extra_scripts = pre:...
# ============================================================================
import gzip
import pathlib

Import("env")  # noqa: F821 — inyectado por PlatformIO/SCons

root = pathlib.Path(env["PROJECT_DIR"])  # noqa: F821
src = (root / "web" / "index.html").read_bytes()
gz = gzip.compress(src, 9, mtime=0)  # mtime=0: salida determinista

lines = [
    "// AUTOGENERADO por tools/gen_webui.py a partir de web/index.html.",
    "// NO editar a mano: se regenera en cada build.",
    "#pragma once",
    "#include <pgmspace.h>",
    f"constexpr size_t WEBUI_GZ_LEN = {len(gz)};",
    "const uint8_t WEBUI_GZ[] PROGMEM = {",
    ",".join(str(b) for b in gz),
    "};",
    "",
]
content = "\n".join(lines)

out = root / "src" / "generated" / "webui_gz.h"
out.parent.mkdir(exist_ok=True)
if not out.exists() or out.read_text() != content:
    out.write_text(content)
    print(f"webui_gz.h regenerado: {len(src)}B html -> {len(gz)}B gzip")
