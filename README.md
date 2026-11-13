# Greenhouse Monitor

Battery and solar greenhouse monitor on a FireBeetle 2 ESP32-C6: light, air temperature and
humidity, and soil moisture and temperature in each pot. It wakes every 10 minutes to take a
reading, and uploads the readings over Wi-Fi once an hour.

**Status:** firmware written for the ESP32-C6 build, waiting on parts. Plan and parts list in
[issue #1](https://github.com/treefitty/greenhouse-monitor/issues/1), wiring in
[docs/wiring.md](docs/wiring.md). The old Witty ESP8266 bench setup is in
[docs/hardware.md](docs/hardware.md), and its firmware is in git history (commit `578a82d`).

## How it runs

- **Timer wake (every 10 min):** read the sensors, store the reading in RTC memory, sleep. Every
  6th wake, connect to Wi-Fi and POST all stored readings as JSON to `UPLOAD_URL`. If that fails,
  they're kept and retried the next hour (up to a day's worth).
- **Power-on, or the button inside the case:** debug session for 5 minutes. The screen shows live
  readings, Wi-Fi flashing is on, and DS18B20 addresses are printed over USB serial. Then it sleeps.

Settings (pot pins, probe addresses, intervals, battery divider) are at the top of `src/main.cpp`.

## Building and flashing

Needs [PlatformIO](https://platformio.org/). The ESP32-C6 toolchain runs natively on Apple Silicon.

One-off setup:

```sh
cp src/secrets.example.h src/secrets.h   # then fill in Wi-Fi, an OTA password and the upload URL
```

```sh
./flash.sh           # build, flash over USB-C, then open the serial monitor
./flash.sh ota       # flash over Wi-Fi. Press the button first: it only listens during a debug session
./flash.sh build     # compile only
./flash.sh monitor   # serial monitor
```

If macOS asks whether Python may accept incoming connections, allow it. The board connects back to
your Mac to download the firmware.

## Layout

```
src/main.cpp     firmware
src/secrets.h    Wi-Fi + OTA passwords (gitignored, from secrets.example.h)
platformio.ini   board + library config
flash.sh         build/flash helper
docs/            hardware notes
```
