#!/bin/sh
# Build in an arm64 Linux container (no native arm64 macOS xtensa toolchain exists),
# then flash natively over USB. Usage: ./flash.sh [build|flash|monitor]
set -e
cd "$(dirname "$0")"
PY=~/.platformio/penv/bin/python

build() {
  docker run --rm --platform linux/arm64 -v "$PWD":/proj -v pio-cache:/root/.platformio -w /proj \
    python:3.12-slim sh -c 'pip -q install platformio && pio run'
}

case "${1:-flash}" in
  build) build ;;
  flash) build && $PY -m esptool write-flash 0x0 .pio/build/esp8266/firmware.bin && pio device monitor ;;
  monitor) pio device monitor ;;
esac
