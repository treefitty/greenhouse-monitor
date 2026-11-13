#!/bin/sh
# Build, then flash over USB or Wi-Fi. Usage: ./flash.sh [build|flash|ota [ip]|monitor]
set -e
cd "$(dirname "$0")"
PIO=~/.platformio/penv/bin/pio
PY=~/.platformio/penv/bin/python
ESPOTA=~/.platformio/packages/framework-arduinoespressif32/tools/espota.py
[ -f src/secrets.h ] || { echo "Missing src/secrets.h: cp src/secrets.example.h src/secrets.h, then fill it in"; exit 1; }
OTA_PASS=$(sed -n 's/^#define OTA_PASS "\(.*\)".*/\1/p' src/secrets.h)

case "${1:-flash}" in
  build) $PIO run ;;
  flash) $PIO run -t upload && $PIO device monitor ;;
  ota) $PIO run && $PY $ESPOTA -i "${2:-greenhouse.local}" -a "$OTA_PASS" -f .pio/build/esp32c6/firmware.bin ;;
  monitor) $PIO device monitor ;;
esac
