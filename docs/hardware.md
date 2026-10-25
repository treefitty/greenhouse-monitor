# Hardware

## Wiring

| OLED | Connects to |
|------|-------------|
| GND  | − rail |
| VCC  | + rail (3.3 V) |
| SCL  | Witty GPIO5 |
| SDA  | Witty GPIO4 |

Witty VCC goes to the + rail and Witty GND to the − rail. Everything shares one ground.

Check the labels printed on the OLED itself. Some modules put VCC first, others GND.

## Power

- Bench supply: **3.3 V**, current limit about **0.3 A**, into the rails.
- Expected draw: about 28 mA for the board alone, a little more with the screen.
- 0.000 A means nothing is connected. If it hits the current limit, there's a short: switch off.
- Keep the OLED on 3.3 V. On 5 V its I2C pull-ups drive the ESP8266 pins above their 3.3 V rating.
- Never power from the bench supply and USB at the same time.

## Breadboard gotchas

- The posts on the Wish board (V1–V3, ground) aren't connected to the rails. Wire them across.
- Rails can be split into sections, and joined breadboards don't share rails. Bridge each gap with a wire.
- Each row of 5 holes is connected together. Make sure two pins of one header never share a row.

## Status LED

The firmware checks for the display over I2C at 0x3C, then 0x3D, and reports on the Witty's RGB LED:

| LED | Meaning |
|-----|---------|
| Green | Display found |
| Red   | Board running, but the display isn't responding. Check SDA/SCL, OLED power and ground |
| Off   | Board isn't running. Check power |

Red on the USB base with no display attached is expected.
