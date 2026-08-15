#!/usr/bin/env bash
# Build full-flash images for browser flashing (ESP Web Tools).
#
# The OTA .bin files hold only the application partition, which is all a device
# needs to update itself but not enough to program a blank chip. ESP8266 images
# are already complete (they start at offset 0), while ESP32 images must be
# merged with the bootloader, the partition table and boot_app0.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/out/webflash}"
VERSION=$(grep -E '^\s*version\s*=' "$ROOT/platformio.ini" | head -1 | sed -E 's/.*=\s*//' | tr -d ' ')
VERSION="${VERSION%-dev}"

ESPTOOL=$(ls "$HOME"/.platformio/packages/tool-esptoolpy/esptool.py 2>/dev/null | head -1)
BOOT_APP0=$(ls "$HOME"/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 2>/dev/null | head -1)

mkdir -p "$OUT"

find_app() {  # $1 = env name
  find "$ROOT/.pio/build/$1" -maxdepth 1 -name 'Firmware_*.bin' -print -quit
}

emit() {      # $1 = env, $2 = mcu label, $3 = chip, $4 = flash size
  local env="$1" mcu="$2" chip="$3" size="$4"
  local app dest
  app="$(find_app "$env")"
  dest="$OUT/ONOFRE_${mcu}_WEBFLASH_${VERSION}.bin"
  if [ -z "$app" ] || [ ! -f "$app" ]; then
    echo "skip $mcu: no build output in .pio/build/$env" >&2
    return 0
  fi
  if [ "$chip" = "esp8266" ]; then
    # Already a complete image starting at offset 0.
    cp "$app" "$dest"
  else
    [ -n "$ESPTOOL" ] || { echo "esptool.py not found" >&2; exit 1; }
    [ -n "$BOOT_APP0" ] || { echo "boot_app0.bin not found" >&2; exit 1; }
    python3 "$ESPTOOL" --chip "$chip" merge_bin -o "$dest" \
      --flash_mode dio --flash_freq 40m --flash_size "$size" \
      0x1000 "$ROOT/.pio/build/$env/bootloader.bin" \
      0x8000 "$ROOT/.pio/build/$env/partitions.bin" \
      0xe000 "$BOOT_APP0" \
      0x10000 "$app" >/dev/null
  fi
  printf '%-18s %8d bytes  %s\n' "$mcu" "$(wc -c < "$dest")" "$(basename "$dest")"
}

emit ESP8266_RELEASE          ESP8266         esp8266 4MB
emit ESP8266-HAN_RELEASE      ESP8266-HAN     esp8266 4MB
emit ESP32_RELEASE            ESP32           esp32   8MB
emit ESP32-MAKER-4MB_RELEASE  ESP32-MAKER-4MB esp32   4MB

echo "webflash images in $OUT (version $VERSION)"
