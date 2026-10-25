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

### Flash over Wi-Fi (normal way)

The board stays on the breadboard. From the project folder:

```sh
./flash.sh ota                  # or: ./flash.sh ota <ip> if greenhouse.local doesn't resolve
```

If macOS asks whether Python may accept incoming connections, allow it. The board connects back to
your Mac to download the firmware.

### Flash over USB (first time, or if Wi-Fi flashing fails)

One-off setup:

```sh
~/.platformio/penv/bin/pip install esptool
cp src/secrets.example.h src/secrets.h   # then fill in Wi-Fi name, password and an OTA password
```

Move the Witty's top board onto its USB base, plug it in, then:

```sh
./flash.sh
```

Move it back to the breadboard. The display shows its IP address once it's on Wi-Fi.

### Other commands

```sh
./flash.sh build     # compile only, no flashing
./flash.sh monitor   # serial monitor (USB base only)
```

## Layout

```
src/main.cpp     firmware
src/secrets.h    Wi-Fi + OTA passwords (gitignored, from secrets.example.h)
platformio.ini   board + library config
flash.sh         build/flash helper
docs/            hardware notes
```
