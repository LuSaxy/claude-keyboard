---
title: Setup guide
outline: deep
---

# Setup guide

From a bare board to a working Clawd in six steps: print the case, wire the
XIAO nRF52840, assemble it, flash the firmware, pair over Bluetooth, and map the
buttons. Set aside an afternoon — most of the time is printing.

## 1 - What you'll need

The full parts list with LCSC numbers, links and prices is on the
**[Parts list (BOM)](/bom)** page. In short:

- **Seeed Studio XIAO nRF52840 Sense** (or the plain XIAO nRF52840). **Not the
  Plus / Sense Plus**: its extra pads bridge the board's pads when soldered.
- The Clawd V2 PCB with its parts: two Kailh hot-swap sockets, two slide
  switches (MODE, POWER), two side-push switches (L1, R1), two yellow LEDs with
  330 Ω resistors, and a JST PH 2.0 battery socket.
- A **protected 1S LiPo** with a JST PH 2.0 plug, up to 40 × 35 × 5 mm.
  **Check its polarity before plugging it in** (see [Battery](/bom#battery)).
- Two MX switches and two 1u keycaps.
- The printed case and a USB-C cable that carries **data**.

## 2 - Print the case

Download the V2 print files from the [Models page](/models) and slice them for
your printer. The 3MF files hold several bodies (one per filament).

| Part | File | Filament |
| --- | --- | --- |
| Top shell + eye covers | `3mf/clawd-v2-top-print.3mf` | Orange shell, clear/yellow translucent eyes (two materials) |
| Bottom shell + USB bezel | `3mf/clawd-v2-bottom-print.3mf` | Orange shell, clear bezel (two materials) |
| Battery plate | `3mf/clawd-v2-battery-plate.3mf` | Any, one colour |

Single-material printer: use the STLs in `stl/` instead and glue the clear
parts in after a dry fit. For a 30 × 37 × 4 mm LiPo use the `-small-lipo`
bottom shell and battery plate.

- PLA or PETG; ~0.2 mm layers, 15–20 % infill.
- **Battery plate:** flat, the two socket-support posts up.
- The L1/R1 shoulder buttons are flexible paddles in the shells: no separate
  parts to print.
- Dry-fit the PCB, battery and plate before soldering anything permanent.

The editable source is `clawd-v2.ket` (open it in Ket). The V1 FreeCAD/3MF files
are still on the Models page for reference; do not mix V1 and V2 parts.

## 3 - Wire the electronics

Buttons and the MODE switch sit between a GPIO and **GND**; the firmware uses
internal pull-ups, so open = HIGH and pressed/closed = LOW. Every ground returns
to the right-side `GND` pin.

| From | To | Notes |
| --- | --- | --- |
| `D0` | Button 1 (left key) → `GND` | Sends Escape by default |
| `D1` | Button 2 (right key) → `GND` | Sends Enter by default |
| `D4` | L1 shoulder → `GND` | V2 board (SW3) · Shift+Tab by default |
| `D5` | R1 shoulder → `GND` | V2 board (SW4) · Tab by default |
| `D2` | MODE switch → `GND` | To GND = USB-HID · open = BLE only |
| `D3` | 330 Ω (300–390 Ω) → LED anode, cathode → `GND` | Active HIGH · one resistor **per** LED (parallel) |
| `BAT+` | ON/OFF switch → LiPo + | Switch cuts the cell when OFF |
| `BAT−` | LiPo − | Underside solder pad |
| USB-C | Power + charge | Automatic — runs the board and charges the cell |

::: danger Check the battery polarity first
JST PH plugs are not wired the same by every seller, and the board has **no
reverse-polarity protection**. A reversed cell gets hot and can damage the
XIAO's charger. On the V2 board, J1 **pin 1 (+)** is the pad next to the small
silkscreen circle. Plug the cell in with POWER off and measure: you must read
**+3.6 to +4.2 V** on that pad against GND. Details on the
[Battery](/bom#battery) page.
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
- Check the battery plug polarity (see above), then lay the LiPo in its cradle
  and drop the battery plate on top; run the leads along the left wall into
  the J1 pocket.
- Run the solder self-test (`make selftest`) and fix anything it reports.
- Power-on test **before** closing: plug in USB-C and confirm a short blue LED
  pulse (advertising). With POWER on, the small red **CHG** LED next to the
  USB-C connector lights while the cell charges.
- Close the top onto the bottom shell.

::: tip Two switches: POWER and MODE
**POWER** (right, next to R1) connects the battery: OFF cuts it, and the battery
does not charge while OFF. **MODE** (left, next to L1) decides whether the USB
cable is also a keyboard: *USB* = it types over the cable, *BLE* = the cable only
charges. Bluetooth works in both positions. MODE is read once at boot, so press
RESET after flipping it. The firmware docs explain how to find which position is
which.
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
Clear an old bond with a **bond reset**: hold *both keys* (B1 + B2) while
powering on (or right after RESET) ~5 s until the LED blinks red 3×, then pair
again.
:::

::: tip Settings stuck after a firmware update?
Flashing new firmware keeps the saved settings (key mapping, name, gestures),
so new defaults do not show if older ones are saved. Wipe them with a
**settings reset**: hold *both shoulders* (L1 + R1) at power-on ~5 s until the
LED blinks blue 3×. Bonds are kept. The control app has the same as
*RESET ALL*, and the serial monitor accepts `R` (`s` prints what is saved).
:::

### Status LED cheat-sheet

| LED | Meaning |
| --- | --- |
| Blue pulse | Advertising / waiting for a connection |
| Green flash | A key was sent, or a pairing was accepted |
| Magenta (steady) | Confirm a new pairing — press Button 1 |
| Red flash | Pairing rejected / failed |
| Red ×3 at boot | Stored bonds wiped (bond reset, B1 + B2) |
| Blue ×3 at boot | Saved settings wiped (settings reset, L1 + R1) |

## That's it

The button mapping, name, and LED all live in the firmware's flash and survive
reboots and deep sleep. Tweak them any time from the [control app](/app).
