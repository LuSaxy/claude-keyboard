| Supported Target | XIAO nRF52840 | XIAO nRF52840 Sense |
| ---------------- | ------------- | ------------------- |

# Clawd Keyboard (XIAO nRF52840) — BLE + USB, battery

Turns a **Seeed Studio XIAO nRF52840** into a tiny **2-key keyboard with two
shoulder buttons** (L1/R1, on the V2 board) that can
type over **Bluetooth LE** *or* over the **USB cable**, runs from a **LiPo
battery**, charges it, and **deep-sleeps** to last for months.

- **BLE HID** via the **Adafruit Bluefruit** stack (`BLEHidAdafruit`).
- **USB HID** via **Adafruit TinyUSB** (`Adafruit_USBD_HID`).
- A **MODE switch** picks USB-HID vs BLE-only; an **ON/OFF switch** cuts the
  battery; a **protected LiPo + firmware low-voltage cutoff** keep the cell safe.
- **System OFF** deep sleep (~single-digit µA) on battery; a button press wakes it.
- **Confirmed pairing**: a new BLE bond must be approved on the device with the
  buttons, so strangers can't silently pair.

By default Button 1 (left key) sends **Escape**, Button 2 (right key) **Enter**, L1 **Shift+Tab**
and R1 **Tab**. Each button is a `modifier + keycode` pair you change in
`xiao_ble_keyboard.ino` or over BLE. Without shoulder buttons wired (V1 build),
D4/D5 just stay HIGH through the pull-ups and do nothing.

> Reference: [Seeed Studio XIAO nRF52840 wiki](https://wiki.seeedstudio.com/XIAO_BLE/)

## Hardware / Wiring

> **On the V2 PCB all of this is already on the board**: you only plug in the
> battery. This section is the pin reference, and the wiring for a hand-wired
> (V1-style) build.

Two momentary push-buttons (the only onboard button is RESET), one **MODE** slide
switch, one **ON/OFF** slide switch, and a single-cell **protected LiPo**.
Buttons and the MODE switch go between a GPIO and **GND** — internal pull-ups are
used, so released/open = HIGH, pressed/closed = LOW.

**Pinout** (XIAO nRF52840, top view, USB-C at the top — same pad layout as every
XIAO board). Left column = `D0..D6`, right column = power + `D7..D10`:

```
                        ┌───────────────────┐
                        │       USB-C       │
                        ├───────────────────┤
    D0  P0.02 (A0)  ────┤                   ├────  5V    (USB 5V out)
    D1  P0.03 (A1)  ────┤   XIAO nRF52840   ├────  GND   ← common ground
    D2  P0.28 (A2)  ────┤                   ├────  3V3   (3.3V out)
    D3  P0.29 (A3)  ────┤    (top view,     ├────  D10  P1.15 (MOSI)
    D4  P0.04 (SDA) ────┤     USB at top)   ├────  D9   P1.14 (MISO)
    D5  P0.05 (SCL) ────┤                   ├────  D8   P1.13 (SCK)
    D6  P1.11 (TX)  ────┤                   ├────  D7   P1.12 (RX)
                        ├───────────────────┤
                        │  BAT+ ●     ● BAT−│   solder pads (underside)
                        └───────────────────┘

  Connections used by this firmware  (every GND goes to the right-side GND pin)
  ───────────────────────────────────────────────────────────────────────────
   D0  ──[ button 1 ]──────────── GND
   D1  ──[ button 2 ]──────────── GND
   D4  ──[ L1 shoulder ]────────── GND        V2 board: SW3 (side push)
   D5  ──[ R1 shoulder ]────────── GND        V2 board: SW4 (side push)
   D2  ──[ MODE slide switch ]─── GND        to GND = USB-HID, open = BLE only

   D3  ──┬──[ 330Ω ]──►|── GND               LED 1   (BLE-controlled, active HIGH)
         └──[ 330Ω ]──►|── GND               LED 2
            ►|  = LED: long leg (anode) to the resistor,
                       short leg / flat side (cathode) to GND

   BAT+ ──[ ON/OFF slide switch ]── LiPo +   (cuts the battery when OFF)
   BAT− ──────────────────────────── LiPo −
   USB-C  ── powers the board AND charges the cell automatically (BQ25101)
```

> The two user LEDs are wired **in parallel, each with its own 330 Ω resistor**
> (never one shared resistor). They share `D3`, so one BLE command drives both.
> Don't put the LEDs in **series**: 2× their forward voltage exceeds the 3.3 V rail
> and they won't light. `D6..D10` stay free for more parts.

| Signal      | Pin         | Chip pin | Notes                                  |
| ----------- | ----------- | -------- | -------------------------------------- |
| Button 1    | D0          | P0.02    | to GND                                 |
| Button 2    | D1          | P0.03    | to GND                                 |
| L1 shoulder | D4          | P0.04    | to GND (V2: SW3)                       |
| R1 shoulder | D5          | P0.05    | to GND (V2: SW4)                       |
| MODE switch | D2          | P0.28    | to GND = USB-HID; open = BLE only      |
| User LED(s) | D3          | P0.29    | BLE-controlled LED(s), active HIGH, PWM |
| Battery +   | BAT+        | -        | via ON/OFF switch                      |
| Battery −   | BAT−        | -        | -                                      |
| Bat. sense  | P0.31 / P0.14 | -      | internal (enable + ADC), used in firmware |
| Charge sel. | P0.13       | -        | internal: HIGH = 50 mA, LOW = 100 mA   |

Notes:
- D0..D5 are plain GPIOs (D3 = the user LED here); D6..D10 stay free.
- **Power source is automatic**: USB present → board runs from USB and charges the
  cell; USB absent → runs from battery. No switch needed for that.
- The **ON/OFF switch only cuts the battery** — with USB plugged in the board
  still powers up (and charges). That's expected; use it for storage / true-off
  on battery.
- The onboard **RGB LED is active LOW**; the sketch uses it as a status indicator.
- **Polarity:** there is **no reverse-polarity protection** on `BAT±` — verify
  the cell's +/− with a meter before connecting.

### Status LED

| LED                  | Meaning                                        |
| -------------------- | ---------------------------------------------- |
| Short **blue** pulse | Advertising / waiting for connection           |
| **Off**              | Connected and idle (low power)                 |
| **Green** flash      | A key was sent, or a pairing was accepted      |
| **Magenta** (steady) | New pairing waiting for confirm (see below)    |
| **Red** flash        | Pairing rejected / failed                      |
| **Red** ×3 (at boot) | Stored bonds wiped (bond reset, B1 + B2)       |
| **Blue** ×3 (at boot)| Saved settings wiped (settings reset, L1 + R1) |

### User LED (BLE controlled)

**External LED(s) on `D3` (P0.29)** that a connected host can switch on/off and dim
over BLE — independent of the status RGB LED. Wire **active HIGH**, and put each
LED on its **own** resistor (parallel), so they light evenly:

```
   D3 ──┬──[ 330 Ω ]──►|── GND      LED 1
        └──[ 330 Ω ]──►|── GND      LED 2
        ►| = LED: long leg (anode) → resistor, short leg (cathode) → GND
```

One LED is fine too (just drop a branch). For two LEDs that's ~4 mA each → ~8 mA on
`D3` (safe; use 470 Ω each for ~6 mA total if you want more margin). **Don't wire
the LEDs in series** — 2× their forward voltage exceeds the 3.3 V rail and they
won't light. Both share `D3`, so the BLE command drives them together.

The firmware exposes a tiny custom **GATT service** with one writable/readable byte
= **brightness** (`0x00` = off … `0xFF` = full, PWM-dimmed in between):

| UUID            | Value                                  |
| --------------- | -------------------------------------- |
| Service         | `494dceab-1533-4012-afaa-20d5acfc6ecf` |
| Characteristic  | `bc6df096-9c5a-4034-89d2-8440c14d2ab0` |

The characteristic requires an **encrypted link**, so only a **bonded** host can
read/write it (matching the project's confirmed-pairing model). It is **not** put
in the advertising packet (no room beside the name + HID UUID); hosts find it by
**GATT service discovery after connecting**.

To test with **nRF Connect**: pair + confirm (Button 1) → connect → open the
service → write the characteristic:

| Write byte | Effect            |
| ---------- | ----------------- |
| `0xFF`     | full brightness   |
| `0x80`     | ~half brightness  |
| `0x00`     | off               |

Resistor value: `R = (3.3 V − Vf) / I`. For red/yellow/green LEDs (Vf ≈ 2.0 V) at
~4 mA → ~330 Ω (use 220 Ω brighter / 470 Ω dimmer). For blue/white (Vf ≈ 3.0 V)
use ~150–220 Ω. Keep each pin under ~8–10 mA total.

Tuning in `xiao_ble_keyboard.ino`: `LED_USER_PIN` (default `D3`) and
`LED_USER_DEFAULT` (boot brightness, default `0`). Brightness is always driven via
`analogWrite()` (PWM), so writing `0x00` = 0 % duty = LED off. The LED resets to
off on deep sleep (the board powers down) and on every reboot.

### Configurable keys (BLE)

What each button sends can be changed **at runtime over BLE** and is **saved to
internal flash**, so it survives reboot and deep sleep. A bonded host writes a
small custom service; the mapping supports **keyboard keys/combos** and
**media/consumer keys**.

| UUID                           | Value                                  |
| ------------------------------ | -------------------------------------- |
| Service                        | `e0934030-b408-4dcc-b4ca-56d83d7c5fbc` |
| Keymap characteristic (RW, 16 B) | `e0c5d214-8809-4243-afa2-9862d3628c38` |
| Device name characteristic (RW) | `8adf91b2-c9a1-4c1a-8b76-2ff5be6b86de` |
| Command characteristic (W)     | `37508895-a542-4ee0-9ab4-b109ce4717de` |

These characteristics require an **encrypted link** (bonded host only). They are
discovered via **GATT after connecting** (not in the advertising packet).

**Keymap = 16 bytes**, 4 per button in the order button 1, button 2, L1, R1
(`code` is little-endian 16-bit):

```
[ t1, m1, c1_lo, c1_hi,  t2, m2, c2_lo, c2_hi,  tL, mL, cL_lo, cL_hi,  tR, mR, cR_lo, cR_hi ]
   │   │     └─ button 1 key code (HID_KEY_* or HID_USAGE_CONSUMER_*)
   │   └─ button 1 modifier bitmask (keyboard only)
   └─ button 1 type: 0 = keyboard, 1 = media/consumer
```

An **8-byte write** (older apps) sets only buttons 1 and 2 and leaves L1/R1 as
they are. A keymap saved by older firmware is kept on upgrade; L1/R1 then start
with their defaults.

- **type** `0` = keyboard (`mod` + `code`), `1` = media/consumer (`code` only, **BLE only**).
- **modifier bits**: LCtrl `0x01`, LShift `0x02`, LAlt `0x04`, LGui `0x08`
  (Right variants `0x10`/`0x20`/`0x40`/`0x80`). Combine with OR.

Examples (8-byte hex to the keymap characteristic):

| Buttons (B1 / B2 [/ L1 / R1])            | Bytes                                             |
| ---------------------------------------- | ------------------------------------------------- |
| `A` / `Enter` (B1/B2 only)               | `00 00 04 00 00 00 28 00`                         |
| `Ctrl+Shift+T` / `Esc` (B1/B2 only)      | `00 03 17 00 00 00 29 00`                         |
| `Volume +` / `Play-Pause` (B1/B2 only)   | `01 00 E9 00 01 00 CD 00`                         |
| defaults `Esc` / `Enter` / `Shift+Tab` / `Tab` | `00 00 29 00 00 00 28 00 00 02 2B 00 00 00 2B 00` |
| `Enter` / `Down` / `Vol−` / `Vol+`       | `00 00 28 00 00 00 51 00 01 00 EA 00 01 00 E9 00` |

Handy **keyboard** codes (HID usage IDs): `A`=`04` … `Z`=`1D`, `1`=`1E` … `0`=`27`,
`Enter`=`28`, `Esc`=`29`, `Tab`=`2B`, `Space`=`2C`, `F1`=`3A` … `F12`=`45`,
`F13`=`68` … `F24`=`73`.
Handy **media** codes (consumer usages): Vol+ `E9`, Vol− `EA`, Mute `E2`,
Play/Pause `CD`, Next `B5`, Prev `B6`.

**Reset to defaults:** write `0x01` to the **command** characteristic — restores the
compiled `BUTTON_*` defaults and saves them.

### Configurable device name (BLE)

The advertised BLE/USB device name defaults to **`clawd`**. A bonded host can read
or write the **Device name** characteristic in the config service:

- write a printable ASCII name, max **15 bytes**;
- the value is saved to internal flash;
- BLE advertising uses the new name after disconnect/reconnect;
- USB-HID uses the new name after reboot, because USB descriptors are fixed at boot.

Write `0x02` to the **command** characteristic to reset the name back to `clawd`.

> Media/consumer keys are sent over **BLE only** (the USB-HID interface here is a
> plain keyboard). Mapped to a media key while in USB mode, a button does nothing
> unless a BLE host is also connected.

### Controlling it with nRF Connect

> **No need to "Forget" the keyboard first.** The firmware accepts two BLE
> links at once and keeps advertising while one is free, so the control app (or
> nRF Connect) connects while your computer is still using Clawd as a keyboard.
> Keys go to every connected host that uses it as a keyboard.

The encrypted LED + config characteristics are easiest to drive from the free
**nRF Connect for Mobile** app (Nordic, Android/iOS), which handles the bonded
pairing properly.

1. **Bond first.** Pair with **clawd** (from the app, or your phone's Bluetooth
   settings); on the device the LED turns **magenta** → press **Button 1 (D0)** to
   confirm. Encrypted reads/writes only work once bonded.
2. **Connect.** nRF Connect → **SCANNER** → *clawd* → **CONNECT**. The custom
   services show up as *Unknown Service*; identify them by UUID:
   - LED service `494dceab-1533-4012-afaa-20d5acfc6ecf`
   - Config service `e0934030-b408-4dcc-b4ca-56d83d7c5fbc`
3. **Write / read.** Each characteristic row has **↑** (write) and **↓** (read).
   In the write dialog pick **BYTE ARRAY / HEX** for LED/keymap/command writes, or
   **TEXT / UTF-8** for the device name, then **SEND**. The first encrypted write
   triggers an Android pairing prompt → accept, then confirm with **Button 1**.

| What            | Characteristic                         | Write                      |
| --------------- | -------------------------------------- | -------------------------- |
| LED on / off    | `bc6df096-…-8440c14d2ab0` (LED)        | `FF` = full, `80` = half, `00` = off |
| Key mapping     | `e0c5d214-…-9862d3628c38` (keymap, 16B; 8B = B1/B2 only) | e.g. `0000040000002800`    |
| Device name     | `8adf91b2-…-be6b86de` (name, text)     | e.g. `clawd-ray` as UTF-8/text |
| Reset keys      | `37508895-…-b109ce4717de` (command)    | `01`                       |
| Reset name      | `37508895-…-b109ce4717de` (command)    | `02`                       |
| Reset all settings (reboots) | `37508895-…-b109ce4717de` (command) | `03`          |

The 8-byte keymap layout, modifier bits and key/media codes are listed under
[Configurable keys (BLE)](#configurable-keys-ble) above. Examples (write to the
keymap characteristic):

| Buttons                        | Bytes              |
| ------------------------------ | ------------------ |
| `A` / `Enter`                  | `0000040000002800` |
| `Ctrl+Shift+T` / `Esc`         | `0003170000002900` |
| `Volume +` / `Play-Pause`      | `0100E9000100CD00` |

Use **↓ (read)** to check the stored value. A written mapping is auto-saved to the
board's flash, so it survives reboot and deep sleep.

## Toolchain setup (Arduino IDE)

1. Install the **Arduino IDE** (2.x recommended).
2. **File > Preferences > Additional Boards Manager URLs**, add:
   ```
   https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
   ```
3. **Tools > Board > Boards Manager...**, search `seeed nrf52`, and install
   **Seeed nRF52 Boards**.
4. **Tools > Board**, select **Seeed XIAO nRF52840** (or *Sense* if you have the
   Sense variant).
5. Connect the board over USB-C and pick the port under **Tools > Port**.
6. **Library Manager** (only if `POWERDOWN_FLASH 1`, the default): install
   **Adafruit SPIFlash** and **SdFat - Adafruit Fork**. These let the sketch put
   the external flash into deep power-down before sleeping. Set
   `POWERDOWN_FLASH 0` in the sketch to skip them (slightly higher sleep current).
   (USB-HID needs no extra library — **Adafruit TinyUSB** ships with the board
   package; just keep **Tools > USB Stack: "Adafruit TinyUSB"**.)

> If `Serial` fails to compile, make sure the sketch keeps
> `#include <Adafruit_TinyUSB.h>` (already included here) — this is a known quirk
> of the *Seeed nRF52 Boards* package.

## Build & Flash

> Pick the FQBN that matches **your** board. Check which one is connected with
> `arduino-cli board list` — the XIAO nRF52840 Sense reports
> `Seeeduino:nrf52:xiaonRF52840Sense`, the plain one
> `Seeeduino:nrf52:xiaonRF52840`, the Plus `Seeeduino:nrf52:xiaonRF52840Plus`
> (USB id `2886:8064`). The examples below use the Sense FQBN.

### Arduino IDE
Open `xiao_ble_keyboard.ino`, choose your board + port, click **Upload**.

### arduino-cli (one-time setup)

Need to have arduino-cli and python

```sh
brew update
brew install arduino-cli
```

```sh
arduino-cli config add board_manager.additional_urls \
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
arduino-cli core update-index
arduino-cli core install Seeeduino:nrf52
```

### arduino-cli — normal upload
```sh
# see which board/port is connected
arduino-cli board list

# compile + upload in one step (run from this folder)
arduino-cli compile --upload -p /dev/ttyACM0 \
  -b Seeeduino:nrf52:xiaonRF52840Sense .
```

```sh
# upload a prebuilt .zip (from Arduino IDE or arduino-cli compile) to the board:
# change folder with result of compile, and the port to match your system.
arduino-cli upload -i /Users/rytsh/Library/Caches/arduino/sketches/6C512CF09F895044ED0AA7456872BEE2/xiao_ble_keyboard.ino.zip -p /dev/cu.usbmodem101 \
  -b Seeeduino:nrf52:xiaonRF52840Sense .
```

This resets the board into its UF2 bootloader automatically (1200-baud "touch")
and flashes it with `adafruit-nrfutil`.

### Manual / two-step upload (WSL, USB-IP, or when auto-reset fails)

In WSL/USB-IP the automatic reset often **drops the USB device** mid-upload (the
port disappears and the flash fails). Do it in two explicit steps instead — put
the board in the bootloader yourself, then flash **without** another reset:

```sh
# 1) Build a DFU package (.zip) into a known folder
arduino-cli compile -b Seeeduino:nrf52:xiaonRF52840Sense \
  --output-dir /tmp/xiao_build .

# 2) Put the board in bootloader mode: double-tap the RESET button.
#    (In WSL re-attach it afterwards, e.g. on Windows:
#       usbipd attach --wsl --busid <id> )
#    Confirm it's in DFU mode — lsusb shows 2886:0045 and a /dev/ttyACM* exists:
arduino-cli board list
lsusb | grep 2886

# 3) Flash the package directly to that port (no touch / no reset)
adafruit-nrfutil dfu serial \
  -pkg /tmp/xiao_build/xiao_ble_keyboard.ino.zip \
  -p /dev/ttyACM0 -b 115200 --singlebank
```

On success it prints `Device programmed.` and the board resets into your sketch
(its USB id flips back from `2886:0045` (bootloader) to `2886:8045` (app)).

`adafruit-nrfutil` ships with the board package; if it isn't on your `PATH`:
`pip install --user adafruit-nrfutil` (and ensure `python` resolves to Python 3).

### UF2 drag-and-drop (no tools)
Double-tap **RESET** → a USB drive named `XIAO-BOOT` mounts → copy a `.uf2` onto
it. (Arduino/arduino-cli produce a `.hex`/`.zip`, not a `.uf2`, so this path is
mainly for prebuilt UF2 images.)

If an upload hangs or the port vanishes, double-tap **RESET** to force the
bootloader and retry the manual steps above.

## Pairing & security

The board does **not** let just anyone bond to it. A new pairing must be
**confirmed on the device with the buttons**, so a stranger can't silently pair
and read your keystrokes. Once a host is bonded it reconnects automatically
(no confirmation needed).

To pair a new device:

1. After flashing, the board advertises as **"clawd"** (short blue pulse).
2. On your phone/PC open Bluetooth settings and pair with **clawd**.
3. The LED turns **magenta** — the board is asking you to confirm:
   - **Button 1 (D0) = accept** → pairs (green flash).
   - **Button 2 (D1) = reject** → cancels (red flash).
   - Do nothing for 30 s → rejected.
4. Open any text field and press a button; the key is sent (green flash).

**Forget all devices (bond reset):** hold **both keys (B1 + B2)** while
powering on / right after RESET; keep holding ~5 s until the LED blinks **red
3×**. All stored bonds are wiped, so you can pair a fresh device.

**Wipe saved settings (settings reset):** hold **both shoulders (L1 + R1)**
while powering on / right after RESET; keep holding ~5 s until the LED blinks
**blue 3×**. The key mapping, device name and gesture switches go back to the
compiled defaults and the board reboots. Bonds are kept.

Flashing new firmware does **not** erase these settings (they live in a small
file system in the chip's flash, so remaps survive updates). If new defaults do
not show after an update, older saved settings are still in use: wipe them as
above, with *RESET ALL* in the control app (command `03`), or with `R` in the
serial monitor (`make monitor`). `s` there prints what is saved, and the
firmware prints it once a couple of seconds after the monitor opens.

How it works: the firmware sets `DisplayYesNo` IO capabilities + MITM and uses a
`pairing-confirm` callback, so modern hosts run BLE numeric comparison and call
back into the device for a yes/no. We have no display, so the buttons are the
"yes/no" — this gives a **physical confirmation** (it is not full MITM protection,
which would need a screen to compare the 6-digit code). Tune with
`REQUIRE_PAIR_CONFIRM`, `PAIR_CONFIRM_TIMEOUT_MS`, `BOND_RESET_HOLD_MS`.

> Older "NoInputNoOutput" hosts may negotiate Just Works and skip the prompt.
> For phones/PCs this is rare; the confirmation prompt is the normal path.

## Motion gestures (XIAO nRF52840 Sense)

With a **XIAO nRF52840 Sense** the built-in motion sensor (LSM6DS3TR-C) adds
these gestures. Build with the Sense board (`Seeeduino:nrf52:xiaonRF52840Sense`);
on other boards the code is left out. Use the plain Sense, not the Sense
**Plus**: its extra pads bridge the board pads (see `../clawd_selftest`).

| Gesture | Default key | How it works |
| ------- | ----------- | ------------ |
| **Lift to wake** | – | While asleep the sensor runs at 26 Hz in low power; moving Clawd wakes it, so it is connected before the first press |
| **Double tap** on the case | Escape | Two quick taps on any side |
| **Tilt** (off by default) | Arrows: left/right, forward = Up, back = Down | Hold **L1 + R1** and lean Clawd |

**Tilt is off by default**; switch it on in the control app (left/right and
forward/back separately). It only works while **L1 and R1 are both held**, so
it never fires while Clawd is just lying on the desk or in your hand:

- the pose when you press the two shoulders is "level";
- lean more than 15° and the arrow key goes out at once, then repeats: slowly
  just past 15°, faster as you lean further (fastest from 40°), like a
  joystick;
- on a diagonal the larger lean wins;
- release the shoulders and it stops. Holding L1 + R1 sends no Tab keys.

With tilt on, single L1 / R1 presses go out 120 ms late, so the chord can be
told apart from a single press. With tilt off they are sent at once.

The gestures are five more entries in the keymap (36 bytes, order B1, B2, L1,
R1, TAP, TILT LEFT, TILT RIGHT, TILT FORWARD, TILT BACK; shorter writes from
older apps still work) and the control app shows them when a Sense is
connected. Each gesture group can be switched off there; the setting is saved
(Motion characteristic `8f1e6c2a-4b7d-4e2b-9a61-3c5d2e8b7f10`: byte 0 = bit0
wake, bit1 tap, bit2 tilt left/right, bit3 tilt forward/back; byte 1 = 1 when
the sensor was found).

**Axes:** the module sits upside down under the PCB. The defaults in
`motion.h` are measured on the first Sense build (leaning right raises sensor
X, leaning forward lowers sensor Y). To check another board, open the serial
monitor (`make monitor`): a `tilt roll=... pitch=...` line shows the angles
twice a second.

- Lean Clawd to its **right**: `roll` must go positive. If it goes negative,
  flip `MOTION_TILT_SIGN`.
- Lean its top **away from you**: `pitch` must go positive. If not, flip
  `MOTION_PITCH_SIGN`.
- If leaning right moves `pitch` instead of `roll`, swap `MOTION_TILT_AXIS`
  (0 ↔ 1).

All three are in `motion.h`, as are `MOTION_TILT_DEG`, `MOTION_TILT_FAST_DEG`,
`MOTION_REPEAT_SLOW_MS`, `MOTION_REPEAT_FAST_MS`, `MOTION_TAP_THS` and
`MOTION_WAKE_THS`.

## Modes & switches

There are two slide switches. On the V2 case both stick out of the back wall:
seen from the front (keys towards you), **MODE is the left one (next to L1)** and
**POWER is the right one (next to R1)**.

| Switch | What it does | Position A | Position B |
| ------ | ------------ | ---------- | ---------- |
| **POWER** (SW2) | Connects or cuts the LiPo | **ON**: runs from the battery | **OFF**: battery disconnected (storage). With USB plugged in the board still runs and the battery does **not** charge. |
| **MODE** (SW1, D2) | Is the USB cable also a keyboard? | **USB**: over the cable the computer sees a keyboard named *clawd* | **BLE**: the cable only charges; typing goes over Bluetooth |

Bluetooth always works in both MODE positions. MODE only decides whether the
USB cable is a keyboard too. It is read **once at boot**: after flipping it,
press **RESET** or replug the cable.

Which way is which depends on the switch part, so check once:

- **MODE:** plug into a computer and press RESET. If a keyboard called *clawd*
  appears (macOS: System Information → USB) and the keys type over the cable,
  it is in the **USB** position; otherwise it is **BLE**.
- **POWER:** unplug USB. If the blue LED pulses (or a key works), it is **ON**;
  if nothing lights up, it is **OFF**.

Mark the positions on the case once you know them.

| MODE switch (D2) | USB cable to PC | What happens                                       |
| ---------------- | --------------- | -------------------------------------------------- |
| **USB** (closed) | plugged         | Types over **USB-HID**, charges, never sleeps      |
| **USB** (closed) | unplugged       | Types over **BLE**, on battery, deep-sleeps        |
| **BLE** (open)   | plugged         | **Only charges** (not seen as a keyboard) + BLE typing |
| **BLE** (open)   | unplugged       | Types over **BLE**, on battery, deep-sleeps        |

So if you only want to *charge from a computer* without it acting as a keyboard,
leave the switch in the **BLE** position. The default USB/BLE name is **clawd**.

Why boot-time? The USB descriptors are fixed when the USB stack starts, so
enabling/disabling the USB keyboard interface can only happen on (re)boot.

## Power management (deep sleep)

To make a LiPo cell last for **months**, the firmware drops the nRF52840 into
**System OFF** — its deepest sleep state (single-digit µA) — whenever the board is
idle **and running on battery**. The on-board QSPI flash is put into deep
power-down first. While USB-powered it never sleeps (it's not draining the cell).

When does it sleep? (battery only)

| Situation                                            | Sleep after      | Knob            |
| ---------------------------------------------------- | ---------------- | --------------- |
| Connected, but no button pressed                     | `IDLE_SLEEP_S`   | default 600 s   |
| Advertising, but nobody connects                     | `ADV_TIMEOUT_S`  | default 180 s   |
| Battery voltage below `VBAT_CUTOFF_MV`               | next measurement | default 3200 mV |

**Waking up:** press any button (keys or shoulders). System OFF can only be left by a reset, so
the board **reboots**, re-advertises, and reconnects in ~1-3 s. The button press
that *wakes* the board is consumed by the wake-up (it is **not** typed) — once
the LED stops pulsing and you're connected, press again to send a key. This is
the standard trade-off for the lowest possible idle current.

All four buttons are configured as `INPUT_PULLUP_SENSE`, so the GPIO SENSE/DETECT
block triggers the wake when a pin is pulled LOW.

Tuning (top of `xiao_ble_keyboard.ino`):
- `IDLE_SLEEP_S` — connected idle timeout. Set `0` to never sleep while connected
  (handy while developing, worse for battery).
- `ADV_TIMEOUT_S` — how long to keep advertising before sleeping.
- `POWERDOWN_FLASH` — `1` (default) deep-power-downs the external flash for the
  lowest current; needs the *Adafruit SPIFlash* + *SdFat - Adafruit Fork*
  libraries. Set `0` to drop the dependency (a few µA higher in sleep).
- Lower `Bluefruit.setTxPower(4)` toward `0` to shave a little radio power at the
  cost of range.

> Power tips: use the **ON/OFF switch** to fully disconnect the cell for storage,
> keep the buttons wired to GND (not 3V3), and remember the board only deep-sleeps
> on battery (USB power keeps it awake by design).

## Customisation

Edit the configuration block at the top of `xiao_ble_keyboard.ino`:

- **What each button sends**: every button is a `modifier + keycode` pair, so
  plain keys *and* combos work the same way (over both USB and BLE):
  ```cpp
  // button = BUTTON_x_MOD , BUTTON_x_CODE
  #define BUTTON_1_MOD   0              // Escape
  #define BUTTON_1_CODE  HID_KEY_ESCAPE
  ```
  | You want        | `..._MOD`                                            | `..._CODE`         |
  | --------------- | ---------------------------------------------------- | ------------------ |
  | `Enter`         | `0`                                                  | `HID_KEY_RETURN`   |
  | free key `F13`  | `0`                                                  | `HID_KEY_F13`      |
  | plain `a`       | `0`                                                  | `HID_KEY_A`   |
  | digit `9`       | `0`                                                  | `HID_KEY_9`   |
  | Ctrl + 9        | `KEYBOARD_MODIFIER_LEFTCTRL`                          | `HID_KEY_9`   |
  | Ctrl+Shift+T    | `KEYBOARD_MODIFIER_LEFTCTRL\|KEYBOARD_MODIFIER_LEFTSHIFT` | `HID_KEY_T`   |
  | Alt + F4        | `KEYBOARD_MODIFIER_LEFTALT`                           | `HID_KEY_F4`  |
  | Win/Cmd + L     | `KEYBOARD_MODIFIER_LEFTGUI`                           | `HID_KEY_L`   |

  Keycodes are the TinyUSB `HID_KEY_*` names (letters, digits, `F1..F24`,
  `ENTER`, `ESCAPE`, `TAB`, `ARROW_UP`, `DELETE`, …). **Free / unbound keys:**
  `F13`–`F24`, or a "Hyper" combo (Ctrl+Shift+Alt+GUI together) + any key.
- **Media / volume keys** (BLE only here): send consumer-control codes, e.g.
  ```cpp
  blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
  delay(KEY_TAP_MS);
  blehid.consumerKeyRelease();
  ```
  Handy codes: `..._VOLUME_INCREMENT`, `..._VOLUME_DECREMENT`, `..._MUTE`,
  `..._PLAY_PAUSE`, `..._SCAN_NEXT`, `..._SCAN_PREVIOUS`.
- **Device name**: change `DEVICE_NAME`, or write the Device name characteristic
  over BLE and reboot for USB-HID to pick it up.
- **TX power / range**: change `Bluefruit.setTxPower(4)` (max `8`).
- **Sleep behaviour**: `IDLE_SLEEP_S`, `ADV_TIMEOUT_S`, `POWERDOWN_FLASH`
  (see [Power management](#power-management-deep-sleep)).
- **Battery**: `ENABLE_BATTERY`, `VBAT_CUTOFF_MV`, `BATTERY_PERIOD_MS`,
  `CHARGE_100MA`, `VBAT_DIVIDER` (see [Battery](#battery)).
- **Pairing security**: `REQUIRE_PAIR_CONFIRM`, `PAIR_CONFIRM_TIMEOUT_MS`,
  `BOND_RESET_HOLD_MS` (see [Pairing & security](#pairing--security)). Set
  `REQUIRE_PAIR_CONFIRM 0` to allow silent (Just Works) pairing again.
- **Mode switch pin**: `MODE_SWITCH_PIN` (default `D2`).
- **BLE-controlled LED**: `LED_USER_PIN` (default `D3`), `LED_USER_DEFAULT` (boot
  brightness) — see [User LED (BLE controlled)](#user-led-ble-controlled).

## Battery

The XIAO nRF52840 has an onboard **BQ25101** charger. Solder a single-cell LiPo
to the `BAT +/-` pads (through the ON/OFF switch) and it charges automatically
whenever USB-C is connected — the CHARGE LED is on while charging, off when full.

With `ENABLE_BATTERY 1` (default) the firmware:
- reports the level over BLE (Battery Service `0x180F`): measured at boot,
  again a few seconds after a host connects, then every `BATTERY_PERIOD_MS`,
  as a notification so macOS / Windows update the shown percentage. The
  voltage is mapped with a LiPo discharge curve, and changes of 1 % are not
  sent (noise). While USB is plugged in the reading is the charger's output,
  which sags and rises with the charge current, so on USB the level only
  goes up, never down; on battery it follows the cell. The serial monitor
  shows each value with USB / battery and the charger's charging state;
- enforces a **low-voltage cutoff**: on battery, if the cell drops below
  `VBAT_CUTOFF_MV` (default 3200 mV) it enters deep sleep to protect it (recover
  by charging). Implausibly low reads (< 2000 mV) are ignored as a sensor glitch.

How it reads voltage (per the Seeed wiki):
- `P0.14` = **READ_BAT_ENABLE** — held **LOW** so the divider is active (keeping
  it HIGH can push `P0.31` past its 3.6 V limit). The active divider draws ~µA.
- `P0.31` = battery-voltage ADC tap through the on-board resistor divider.

> ⚠️ **Use a protected (PCM) cell.** The BQ25101 does **not** protect against
> over-discharge; the firmware cutoff is a *second* line of defence, not the only
> one.

**Calibration:** `VBAT_DIVIDER` (default `2.9608`) varies slightly between board
revisions. Measure the real cell voltage with a meter, compare to the reported
value, and scale `VBAT_DIVIDER` until they match. If `analogRead(PIN_VBAT)` does
not compile on your variant, adjust the `PIN_VBAT` / `PIN_VBAT_ENABLE` macros (or
set `ENABLE_BATTERY 0`).

**Charge current:** `CHARGE_100MA 0` → 50 mA (gentle, for small cells, default);
`1` → 100 mA. Pick ≤ ~1C for your cell.

## How it works

- At boot the **MODE switch** (D2) is read into `usbMode`. If USB is selected,
  `usb_hid.begin()` registers a USB-HID keyboard; otherwise only BLE/CDC come up.
- `BLEHidAdafruit` registers a standard HID-over-GATT keyboard; the host sees a
  normal Bluetooth keyboard.
- `pollPressed()` debounces each GPIO (15 ms) and fires once on the press edge
  (HIGH→LOW). `sendKey()` then routes the key to **USB-HID if it's ready**, else
  to **BLE**.
- `usbPowered()` reads `NRF_POWER->USBREGSTATUS` (VBUS) to tell whether the board
  is on USB power; deep sleep is skipped whenever it is.
- `readVbatMv()` enables the divider (P0.14 LOW), reads P0.31, applies
  `VBAT_DIVIDER`, updates the BLE Battery Service, and triggers the low-voltage
  cutoff.
- `updateLed()` drives the active-LOW RGB LED for connection / keypress status.
- `enterDeepSleep()` drops the link, deep-power-downs the QSPI flash, arms the
  buttons as `INPUT_PULLUP_SENSE` wake sources, and calls `sd_power_system_off()`.
  A button press then wakes (resets) the chip.

## Troubleshooting

| Symptom                          | Fix                                                        |
| -------------------------------- | --------------------------------------------------------- |
| Not in the Bluetooth list        | Confirm the blue LED is pulsing; reset the board.         |
| Pairing fails / phone gives up   | Watch for the **magenta** LED and press **Button 1** to accept (within 30 s). |
| Can't pair a new phone           | Old bond may block it; do a **bond reset** (hold B1 + B2 ~5 s at boot). |
| New defaults missing after flashing | Saved settings survive flashing; **settings reset** (hold L1 + R1 ~5 s at boot). |
| Paired but no keys after re-pair | "Forget" the device on the host, bond-reset on the board, pair again. |
| Upload hangs / port missing      | Double-tap RESET to enter the `XIAO-BOOT` bootloader.     |
| Upload fails in WSL / USB-IP     | Use the [manual two-step upload](#manual--two-step-upload-wsl-usb-ip-or-when-auto-reset-fails). |
| `Serial` won't compile           | Keep `#include <Adafruit_TinyUSB.h>`.                     |
| `Adafruit_SPIFlash.h` not found  | Install the libraries, or set `POWERDOWN_FLASH 0`.        |
| Mode change ignored              | The MODE switch is read at boot — press RESET after flipping it. |
| Typing over USB but wanted charge-only | Set MODE switch to **BLE** (open), then RESET.      |
| Not typing over USB              | MODE switch must be **USB** (to GND); USB only works to a *data* port. |
| First press does nothing         | It woke the board from deep sleep; press again once connected. |
| Won't wake from sleep            | Button must pull the pin to **GND** (LOW); check wiring.  |
| Reported battery % is off        | Calibrate `VBAT_DIVIDER` against a meter.                 |
| Keys repeat / bounce             | Increase `DEBOUNCE_MS`.                                    |
| Letter appears but wrong char    | Host keyboard layout differs; HID sends key *codes*.      |

## USB in WSL

> https://learn.microsoft.com/en-us/windows/wsl/connect-usb  
> https://github.com/dorssel/usbipd-win/releases

```sh
usbipd.exe list
# if not shared bind first
usbipd.exe bind --busid 1-1
usbipd.exe attach --wsl --busid 1-1
```
