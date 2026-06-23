---
title: Setup guide
outline: deep
---

# Setup guide

From a bare board to a working Clawd in six steps: print the case, wire the
XIAO nRF52840, assemble it, flash the firmware, pair over Bluetooth, and map the
buttons. Set aside an afternoon — most of the time is printing.

## 1 - What you'll need

Clawd is built around a single **Seeed Studio XIAO nRF52840** (the *Sense*
variant works too). The rest are common keyboard and hobby parts — plan on a
little soldering with thin (~28 AWG) wire to connect the battery and switches.

| Qty | Part | Notes |
| --- | --- | --- |
| 1× | XIAO nRF52840 | Or the Sense variant. Brains + radio + USB-C + LiPo charger. |
| 1× | Protected LiPo cell (150–300 mAh) | Single cell with a PCM. Charges over USB-C automatically. |
| 2× | Mechanical keyboard switches | The two keys. The board's only onboard button is RESET. |
| 2× | Kailh hot-swap sockets | Soldered in so the switches drop in/out without re-soldering. |
| 2× | Keycaps (double-layer, transparent cap) | Snap onto the switches; the clear cap holds a custom legend. |
| 1× | 2-position DIP switch | One slider sets USB-HID vs BLE at boot; the other is battery ON/OFF. |
| 2× | Yellow LED | The mascot's eyes — BLE-controlled. |
| 2× | Resistor, 330 Ω (300–390 Ω) | Current-limit for the LEDs — one **per** LED. |
| — | Wire, solder, USB-C cable | Thin (~28 AWG) hookup wire and a **data**-capable USB-C cable for flashing. |
| 1× | Printed enclosure | Top + bottom shells — printed in the next step. |

## 2 - Print the case

The mascot-style enclosure is two parts. Download the printable 3MF files from
the [Models page](/models) and slice them for your printer.

- Print `mascot-top.3mf` and `mascot-bottom.3mf`.
- PLA or PETG is fine; ~0.2 mm layers, 15–20% infill.
- **Bottom shell:** prints without supports — orient it flat-side down.
- **Top shell:** print it top-side **down** and enable supports.
- Test-fit the XIAO, buttons, and switches in the dry print before soldering.

FreeCAD sources (`mascot.FCStd`, `assembly.FCStd`) are on the
[Models page](/models) if you want to tweak the fit.

## 3 - Wire the electronics

Buttons and the MODE switch sit between a GPIO and **GND**; the firmware uses
internal pull-ups, so open = HIGH and pressed/closed = LOW. Every ground returns
to the right-side `GND` pin.

| From | To | Notes |
| --- | --- | --- |
| `D0` | Button 1 → `GND` | Sends F13 by default |
| `D1` | Button 2 → `GND` | Sends F14 by default |
| `D2` | MODE switch → `GND` | To GND = USB-HID · open = BLE only |
| `D3` | 330 Ω (300–390 Ω) → LED anode, cathode → `GND` | Active HIGH · one resistor **per** LED (parallel) |
| `BAT+` | ON/OFF switch → LiPo + | Switch cuts the cell when OFF |
| `BAT−` | LiPo − | Underside solder pad |
| USB-C | Power + charge | Automatic — runs the board and charges the cell |

::: warning Check polarity
There is no reverse-polarity protection on `BAT±` — verify the cell's +/− with a
meter before connecting, and use a protected (PCM) cell.
:::

::: tip LEDs
Never put them in series (2× their forward voltage exceeds the 3.3 V rail). One
LED is fine — just drop a branch. Both LEDs share `D3`, so one BLE command drives
them together.
:::

Full pinout, chip-pin map, and battery-sense details are in the
[firmware wiring docs](/firmware#hardware--wiring).

## 4 - Assemble it

- Seat the XIAO in the bottom shell with the USB-C port lined up to the opening.
- Mount the two buttons through the top shell's key holes and the MODE / ON-OFF
  switches in their slots.
- Tuck the LiPo into the free space; keep wires short and away from the buttons.
- Power-on test **before** closing: plug in USB-C and confirm a short blue LED
  pulse (advertising).
- Close the top onto the bottom shell.

::: tip MODE switch is read once at boot
Leave it on *USB* (to GND) to type over the cable, or *BLE* (open) to only charge
from a PC. Flip it, then press RESET to apply.
:::

## 5 - Flash the firmware

Set up the Arduino toolchain once, then upload the sketch.

1. Install the **Arduino IDE** (2.x).
2. **File → Preferences → Additional Boards Manager URLs**, add:
   ```
   https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
   ```
3. **Tools → Boards Manager**, search `seeed nrf52`, install **Seeed nRF52 Boards**.
4. **Tools → Board** → **Seeed XIAO nRF52840** (or Sense). Keep
   **USB Stack: Adafruit TinyUSB**.
5. Connect over USB-C, pick the port under **Tools → Port**.
6. Open `program/xiao_ble_keyboard/xiao_ble_keyboard.ino` and click **Upload**.

Prefer the command line? Compile and upload with `arduino-cli`:

```sh
arduino-cli compile --upload -p /dev/ttyACM0 \
  -b Seeeduino:nrf52:xiaonRF52840Sense .
```

::: tip Upload hangs or the port vanishes?
Double-tap **RESET** to enter the `XIAO-BOOT` bootloader, then retry. The full
toolchain, WSL/USB-IP notes, and troubleshooting table are in the
[firmware docs](/firmware#build--flash).
:::

## 6 - Pair, then map the buttons

After flashing, the board advertises as **clawd** (short blue pulse). Pairing is
confirmed *on the device* so no one can silently bond to it.

1. Open your phone/PC Bluetooth settings and pair with **clawd**.
2. The LED turns **magenta** — press **Button 1** to accept (green flash) or
   **Button 2** to reject.
3. Open any text field and press a button; the key is sent (green flash).

Now remap the keys, set the LED, or rename the device from the
[control app](/app) — no install, Chrome / Edge over `https://`.

::: tip Pairing stuck?
Clear an old bond with a **bond reset**: hold *both buttons* while powering on
(or right after RESET) ~5 s until the LED blinks red 3×, then pair again.
:::

### Status LED cheat-sheet

| LED | Meaning |
| --- | --- |
| Blue pulse | Advertising / waiting for a connection |
| Green flash | A key was sent, or a pairing was accepted |
| Magenta (steady) | Confirm a new pairing — press Button 1 |
| Red flash | Pairing rejected / failed |
| Red ×3 at boot | Stored bonds wiped (bond reset) |

## That's it

The button mapping, name, and LED all live in the firmware's flash and survive
reboots and deep sleep. Tweak them any time from the [control app](/app).
