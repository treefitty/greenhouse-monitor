# Greenhouse Monitor

ESP8266-based greenhouse environment monitor with an SSD1306 OLED display.

**Status:** the board boots and drives the display (title and uptime counter). Sensors are next.

## Hardware

- Witty Cloud ESP8266 (ESP-12F top board + CH340 USB base)
- SSD1306 128x64 OLED, I2C
- Bench power supply (or the Witty's micro-USB)

Wiring, power and troubleshooting are in [docs/hardware.md](docs/hardware.md).

## Building and flashing

Needs [PlatformIO](https://platformio.org/) and Docker (OrbStack works).

There's no native Apple Silicon macOS build of the ESP8266 compiler. `flash.sh` compiles inside a
`linux/arm64` container, which runs natively without emulation or Rosetta, and then flashes over
USB from macOS with esptool.

```sh
~/.platformio/penv/bin/pip install esptool   # one-off: native flasher

./flash.sh build     # compile only
./flash.sh           # compile, flash, open serial monitor
./flash.sh monitor   # serial monitor only
```

The Witty's top board has no USB serial. To flash it, sit it on its USB base, then move it back to
the breadboard.

## Layout

```
src/main.cpp     firmware
platformio.ini   board + library config
flash.sh         build/flash helper
docs/            hardware notes
```
