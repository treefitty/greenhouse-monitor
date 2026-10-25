# Hardware

## Wiring

| OLED | Connects to |
|------|-------------|
| GND  | − rail |
| VCC  | + rail (5 V) |
| SCL  | Witty GPIO5 |
| SDA  | Witty GPIO4 |

Witty VCC goes to the + rail (5 V) and Witty GND to the − rail. Everything shares one ground.

The OLED runs on 5 V only because most modules have their own 3.3 V regulator (a tiny part marked
662K), which keeps SDA/SCL at 3.3 V. Check with a meter: SDA and SCL to GND should read about 3.3 V.
If they read 5 V, the module has no regulator and needs a separate 3.3 V supply.

Don't power anything from CH_PD. It's the chip-enable pin, pulled up through a resistor: any load
pulls it low and switches the ESP8266 off.

Check the labels printed on the OLED itself. Some modules put VCC first, others GND.

## Power

- Bench supply: **5.0 V**, current limit about **0.5 A**, into the rails.
- Witty VCC is the *input* to its onboard 1117 regulator, not a 3.3 V pin, whatever the pinout diagrams say. Feed it 3.3 V and the chip gets
  about 2.3 V: enough to blink without Wi-Fi, but it won't boot with Wi-Fi (blank screen, LED off,
  steady ~28 mA).
- Expected draw: about 70–80 mA with Wi-Fi on, in short spikes up to about 350 mA while it transmits. A 0.3 A limit can brown out the board.
- 0.000 A means nothing is connected. If it hits the current limit, there's a short: switch off.
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
