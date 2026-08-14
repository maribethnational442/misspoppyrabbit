#!/bin/bash
# ============================================================================
# build-release.sh — genera los artefactos de distribución de Miss Poppy Rabbit
#
#  1. docs/flash/*.bin + manifest.json  → para el Web Installer (ESP Web
#     Tools en GitHub Pages): flasheo con un clic desde Chrome
#  2. release/misspoppyrabbit-vX.Y.Z.bin → binario FUSIONADO (todo en uno,
#     offset 0x0) para M5Burner y para adjuntar al GitHub Release
#
# Uso: ./tools/build-release.sh   (desde la raíz del repo)
# ============================================================================
set -euo pipefail
cd "$(dirname "$0")/.."

PIO="$HOME/.platformio/penv/bin/pio"
PY="$HOME/.platformio/penv/bin/python"
ESPTOOL="$(ls "$HOME"/.platformio/packages/tool-esptoolpy/esptool.py)"
BUILD=".pio/build/cardputer-adv"

VERSION=$(grep -o 'OS_VERSION *= *"[^"]*"' src/core/Config.h | grep -o '[0-9.]*')
echo "== Miss Poppy Rabbit v$VERSION =="

# 1. Compilar (la WebUI se embebe sola via tools/gen_webui.py)
"$PIO" run

BOOT_APP0=$(ls "$HOME"/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin)

# 2. Piezas para el Web Installer (ESP Web Tools necesita servirlas con CORS,
#    por eso viven en docs/, que GitHub Pages sirve desde el propio origen)
mkdir -p docs/flash
cp "$BUILD/bootloader.bin" "$BUILD/partitions.bin" "$BUILD/firmware.bin" docs/flash/
cp "$BOOT_APP0" docs/flash/boot_app0.bin

cat > docs/flash/manifest.json << EOF
{
  "name": "Miss Poppy Rabbit",
  "version": "$VERSION",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        { "path": "bootloader.bin", "offset": 0 },
        { "path": "partitions.bin", "offset": 32768 },
        { "path": "boot_app0.bin", "offset": 57344 },
        { "path": "firmware.bin", "offset": 65536 }
      ]
    }
  ]
}
EOF

# 3. Binarios de release:
#    - fusionado (offset 0x0): M5Burner y esptool a pelo
#    - app solo: instalacion por microSD con M5Launcher
mkdir -p release
MERGED="release/misspoppyrabbit-v$VERSION.bin"
"$PY" "$ESPTOOL" --chip esp32s3 merge_bin -o "$MERGED" \
  --flash_mode dio --flash_size 8MB \
  0x0 "$BUILD/bootloader.bin" \
  0x8000 "$BUILD/partitions.bin" \
  0xe000 "$BOOT_APP0" \
  0x10000 "$BUILD/firmware.bin"
cp "$BUILD/firmware.bin" "release/misspoppyrabbit-v$VERSION-app.bin"

echo ""
echo "== Artefactos listos =="
ls -lh docs/flash/
ls -lh release/
echo ""
echo "Web installer: commit de docs/ + GitHub Pages (main, carpeta /docs)"
echo "M5Burner:      subir el fusionado (flashea en offset 0x0)"
echo "M5Launcher:    copiar el -app.bin a la microSD e instalar desde el menu"
