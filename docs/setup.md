---
title: Build guide
outline: [2, 3]
---

# Build guide

From parts to a working Clawd V2: order the board, print the case, solder,
test, flash, close it up and pair. Plan a few evenings: the PCB takes a week or
two to arrive, the print takes a few hours, the soldering about an hour.

## 1. Get the files

Everything (print files, Gerbers, firmware, self-test) is in the repository:

```sh
git clone https://github.com/LuSaxy/claude-keyboard.git
cd claude-keyboard
git lfs pull   # the model and PCB files are stored in Git LFS
```

No git? [Download the ZIP](https://github.com/LuSaxy/claude-keyboard/archive/refs/heads/main.zip)
for the code, and take the print and PCB files from
[Print files & models](/models).

## 2. Order the parts and the board

The full list with LCSC numbers and links is the [parts list](/bom). The short
version:

| Part | Note |
| --- | --- |
| XIAO nRF52840 **Sense** (or plain) | **Not the Plus / Sense Plus**: its extra pads short to the board's pads. The Sense adds the motion gestures. |
| Clawd V2 PCB | Upload `models/v2/pcb/clawd-v2-gerber.zip` (rev A) at [JLCPCB](https://jlcpcb.com/quote): 2 layers, 1.6 mm. Other settings are your choice. |
| Board parts | 2 Kailh MX hot-swap sockets, 2 slide switches (MODE, POWER), 2 side-push switches (L1, R1), 2 yellow 0805 LEDs, 2 × 330 Ω 0603, 1 JST PH 2.0 socket. |
| LiPo | 1S, **protected**, JST PH 2.0 plug, up to 40 × 35 × 5 mm (or 30 × 37 × 4 mm with the small-LiPo print files). |
| Keys | 2 MX switches, 2 × 1u keycaps. |
| Cable | USB-C with **data** lines: charge-only cables cannot flash the board. |

::: tip Board assembly
You can let the fab place the small top-side parts from the BOM/CPL in
`clawd-v2-jlcpcb-prototype.zip`. Check that they can place the XIAO module; if
not, solder it by hand. Review the placement preview before paying.
:::

## 3. Print the case

| Part | File | Filament |
| --- | --- | --- |
| Top shell + eye covers | `3mf/clawd-v2-top-print.3mf` | Orange shell, clear or yellow translucent eyes |
| Bottom shell + USB bezel | `3mf/clawd-v2-bottom-print.3mf` | Orange shell, clear bezel |
| Battery plate | `3mf/clawd-v2-battery-plate.3mf` | Any, one colour |

- PLA or PETG, about 0.2 mm layers, 15–20 % infill.
- **Top shell** upside down. Keep supports out of the slits around the L1/R1
  paddles; clean them with a knife if they fused.
- **Battery plate** flat, posts up.
- One-material printer: print the STLs from `stl/` and glue the clear parts in
  after a dry fit (sand them slightly; they are drawn with zero clearance).
- Small LiPo (30 × 37 × 4 mm): use the `-small-lipo` bottom shell and plate.

The L1/R1 shoulder buttons are flexible paddles in the shells, not separate
parts. Print both shells from the same revision.

## 4. Set up the firmware tools

You need these before soldering, to run the self-test. Install
[`arduino-cli`](https://arduino.github.io/arduino-cli/latest/installation/),
then once:

```sh
arduino-cli config add board_manager.additional_urls \
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
arduino-cli core update-index
arduino-cli core install Seeeduino:nrf52
arduino-cli lib install "Adafruit SPIFlash" "SdFat - Adafruit Fork"
```

The repository's `Makefile` finds the connected XIAO by itself:

| Command | Does |
| --- | --- |
| `make selftest` | Flash the solder self-test and show its report |
| `make flash` | Flash the keyboard firmware |
| `make monitor` | Serial monitor (`s` prints the settings, `R` wipes them) |

Prefer the Arduino IDE? The steps are in the
[firmware reference](/firmware#toolchain-setup-arduino-ide).

## 5. Solder and self-test

1. **Top side first** (the low parts): the two 330 Ω resistors, the two LEDs
   (cathode mark as on the silkscreen), the slide switches SW1/SW2 and the
   side-push switches SW3/SW4.
2. **Bottom side:** the XIAO, the JST socket J1 and the two hot-swap sockets
   K1/K2. Solder the XIAO castellations with little solder; nothing may reach
   the pads under the module.
3. Plug the board in with **no battery** and run:

   ```sh
   make selftest
   ```

   It must end with **RESULT: PASS**. Then press B1, B2, L1 and R1 and flip
   MODE: each change is printed. "D3 is joined to: D4 D5" means a solder
   bridge between those pads. Details:
   [self-test](https://github.com/LuSaxy/claude-keyboard/tree/main/program/clawd_selftest).

## 6. Check the charger and the battery

::: danger The board has no reverse-polarity protection
JST PH plugs are not wired the same by every seller. A reversed cell gets hot
and destroys the XIAO's charger. Do both checks below, every time you use a new
cell.
:::

1. **Charger, no cell connected.** In the self-test report, **VBAT** must be
   about **4.1–4.2 V**. About 5 V means the charger is damaged: do not connect
   a cell to that XIAO.
2. **Polarity.** J1 **pin 1 (+)** is the pad next to the small silkscreen
   circle on the back. Measure the plug of your cell: the red lead must land on
   pin 1. If not, swap the two pins in the plug housing (lift the latch with a
   needle) before plugging it in.
3. Plug the cell in with **POWER off**, then switch it on. With USB connected
   the cell charges at 50 mA; the small charge LED next to the USB-C port is
   on while it charges.

The full battery guide, with charging and troubleshooting, is on the
[parts page](/bom#battery).

## 7. Flash and close it up

```sh
make flash
```

After flashing, the status LED gives a short **blue** pulse: Clawd is
advertising. Then assemble:

1. Lay the cell in the cradle of the bottom shell and drop the battery plate on
   top. Run the leads along the left wall into the J1 pocket.
2. Put the board in, XIAO and key sockets down: the plate's two posts sit
   under the sockets.
3. Close the top shell: its pegs drop into the board tabs, and five bumps click
   into the bottom lip all round. To open, pull the halves apart at an arm.
4. Push the MX switches through the top opening into the hot-swap sockets,
   then the keycaps.

## 8. Pair and map the keys

1. Pair **clawd** in your computer's or phone's Bluetooth settings.
2. The status LED turns **magenta**: press **B1** (left key) to accept, **B2**
   to refuse. Nobody can pair with Clawd without pressing a key on it.
3. Press a key in any text field: B1 sends Escape, B2 Enter, L1 Shift+Tab,
   R1 Tab.

Remap every button, set the eye brightness and turn on the gestures in the
[control app](/app) (Chrome or Edge on a computer or Android).

### The two switches

| Switch | Position | Effect |
| --- | --- | --- |
| **POWER** (right, next to R1) | I / O | Connects or cuts the battery. The cell only charges with POWER on. |
| **MODE** (left, next to L1) | USB / Bluetooth | *USB*: the cable is also a keyboard. *Bluetooth*: the cable only charges. Bluetooth works in both. Read at start-up: press RESET after changing it. |

The raised icons on the case show which side is which.

### Status LED

| LED | Meaning |
| --- | --- |
| Blue pulse | Advertising, waiting for a connection |
| Off | Connected and idle |
| Green flash | A key was sent, or a pairing was accepted |
| Magenta, steady | Confirm a new pairing: press B1 |
| Red flash | Pairing refused or failed |
| Red ×3 at start-up | Pairings wiped (bond reset) |
| Blue ×3 at start-up | Saved settings wiped (settings reset) |

### When something is stuck

| Problem | Fix |
| --- | --- |
| A new computer cannot pair | **Bond reset:** hold B1 + B2 while switching on (or right after RESET) for about 5 s, until red ×3. Then forget Clawd on the computer and pair again. |
| New firmware defaults do not show | Saved settings survive flashing. **Settings reset:** hold L1 + R1 at start-up for about 5 s, until blue ×3. Pairings are kept. The control app's *RESET ALL* does the same. |
| Upload hangs or the port disappears | Double-tap RESET: a `XIAO-BOOT` drive appears. Run `make flash` again. |
| Not typing over the cable | MODE must be on *USB*, and the cable must carry data. Press RESET after switching. |

More in the [firmware troubleshooting table](/firmware#troubleshooting).
