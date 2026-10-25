#!/bin/sh
# Build in an arm64 Linux container (no native arm64 macOS xtensa toolchain exists),
# then flash natively over USB or Wi-Fi. Usage: ./flash.sh [build|flash|ota [ip]|monitor]
set -e
cd "$(dirname "$0")"
PY=~/.platformio/penv/bin/python
ESPOTA=~/.platformio/packages/framework-arduinoespressif8266/tools/espota.py
[ -f src/secrets.h ] || { echo "Missing src/secrets.h: cp src/secrets.example.h src/secrets.h, then fill it in"; exit 1; }
OTA_PASS=$(sed -n 's/^#define OTA_PASS "\(.*\)".*/\1/p' src/secrets.h)

build() {
  docker run --rm --platform linux/arm64 -v "$PWD":/proj -v pio-cache:/root/.platformio -w /proj \
    python:3.12-slim sh -c 'pip -q install platformio && pio run'
}

case "${1:-flash}" in
  build) build ;;
  flash) build && $PY -m esptool write-flash 0x0 .pio/build/esp8266/firmware.bin && pio device monitor ;;
  ota) build && $PY $ESPOTA -i "${2:-greenhouse.local}" -a "$OTA_PASS" -f .pio/build/esp8266/firmware.bin ;;
  monitor) pio device monitor ;;
esac
