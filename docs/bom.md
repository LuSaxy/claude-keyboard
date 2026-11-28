---
title: Parts list (BOM)
outline: deep
---

# Parts list (BOM)

Everything needed for one **Clawd V2**. The board parts (U1, K1/K2, SW1–SW4,
D1/D2, R1/R2, J1) match `models/v2/pcb/bom.csv`, which is what the PCB was
designed for. The rest are off-board parts.

Prices are **per one Clawd**, in USD. A dash means the price is not filled in
yet. Links are examples, not recommendations; any part with the same footprint
works.

## Board parts

| Ref | Qty | Part | Value / MPN | Footprint | LCSC | Link | Unit price | Line total |
| --- | --- | --- | --- | --- | --- | --- | ---: | ---: |
| U1 | 1 | Seeed Studio XIAO nRF52840 **Sense** (or plain). **Not** the *Plus* / *Sense Plus*, see [Module](#module-xiao-nrf52840-sense-not-plus) | 102010469 (Sense) / 102010448 | XIAO castellated | C37327670 | [Seeed](https://www.seeedstudio.com/Seeed-XIAO-BLE-Sense-nRF52840-p-5253.html) | — | — |
| K1, K2 | 2 | Kailh hot-swap socket, MX | CPG151101S11 | TH, 10.9 × 5.9 mm | C2803348 | [LCSC](https://www.lcsc.com/product-detail/C2803348.html) | — | — |
| SW1, SW2 | 2 | Slide switch SPDT, **2.5 mm** knob (MODE, POWER). Same pads as the G20 (2.0 mm) used on the first builds; the longer knob is easier to reach with a fingernail | MSK12C02G25-B | SMD MSK12C02 | C22435663 | [LCSC](https://www.lcsc.com/product-detail/C22435663.html) | — | — |
| SW3, SW4 | 2 | Side-push tactile switch, 2.2 N (L1, R1) | Panasonic EVQP7C01K | SMD EVQP7C | C528965 | [LCSC](https://www.lcsc.com/product-detail/C528965.html) | — | — |
| D1, D2 | 2 | LED yellow, 0805 (eyes) | SZYY0805Y | 0805 | C434436 | [LCSC](https://www.lcsc.com/product-detail/C434436.html) | — | — |
| R1, R2 | 2 | Resistor 330 Ω, 0603 (one per LED) | 0603WAF3300T5E | 0603 | C23138 | [LCSC](https://www.lcsc.com/product-detail/C23138.html) | — | — |
| J1 | 1 | JST PH 2.0 mm, 2-pin, SMD right angle (battery) | A2001WR-S-2P | SMD 2P 2.00 mm | C375128 | [LCSC](https://www.lcsc.com/product-detail/C375128.html) | — | — |
| PCB | 1 | Clawd V2 PCB, 2 layers, 1.6 mm | `clawd-v2-gerber.zip` | 59.8 × 36 mm | – | [JLCPCB](https://jlcpcb.com/quote) | — | — |

## Off-board parts

| Qty | Part | Notes | Link | Unit price | Line total |
| --- | --- | --- | --- | ---: | ---: |
| 1 | LiPo cell, 1S 3.7 V, **with protection (PCM)**, JST PH 2.0 plug | Up to 40 × 35 × 5 mm (or 30 × 37 × 4 mm with the small-LiPo parts). **Check the plug polarity**, see [Battery](#battery) | [example](https://www.adafruit.com/product/1578) | — | — |
| 2 | Mechanical key switch, MX-compatible | Hot-swap, any MX switch | [example](https://www.aliexpress.com/) | — | — |
| 2 | Keycap, 1u (DSA, double-layer with clear cap) | Clear cap holds a custom legend | [example](https://www.aliexpress.com/) | — | — |
| 2 | Screw for the board tabs (optional) | Self-tapping, about M1.6–M2 × 4–6 mm; head ≤ 4.4 mm | [example](https://www.aliexpress.com/) | — | — |
| – | Filament | Orange shells (PLA/PETG; the L1/R1 paddles are part of them); clear/translucent eyes and USB bezel | – | — | — |
| 1 | USB-C cable with **data** lines | Charge-only cables power the board but cannot flash it | – | — | — |

## Total

| | Price |
| --- | ---: |
| Board parts | — |
| Off-board parts | — |
| **Total for one Clawd** | **—** |

PCB fabs sell boards in lots (usually 5), and LCSC parts often have a minimum
order; the per-Clawd total above is the share of one build.

## Module: XIAO nRF52840 Sense, not Plus

Use the **plain XIAO nRF52840** or the **XIAO nRF52840 Sense**. The Sense adds
the motion sensor used for lift-to-wake, double tap and tilt (see the
[firmware docs](/firmware#motion-gestures-xiao-nrf52840-sense)).

Do **not** use the **Plus** or **Sense Plus** on this board. They have nine
extra small pads on the underside, between the standard castellations. The V2
board has pads for the standard XIAO only, and when the module is reflowed the
solder joins those extra pads to the neighbouring pins (on the first build:
D3, D4, D5, D6 and the battery-sense pin all joined, so L1/R1 never worked).

After soldering the module, flash the **self-test** (`make selftest`) before
closing the case: it lists bridged, shorted and leaking pins.

## Battery

The XIAO charges and protects nothing by itself beyond what its BQ25101
charger does, so the battery deserves care.

### Choosing a cell

- **One cell (1S), 3.7 V nominal, 4.2 V full.** Never a 2S pack.
- **With a protection circuit (PCM).** The small board under the wrap that cuts
  the cell on short circuit, over-discharge and over-charge. The firmware's
  3.2 V cutoff is only a second line of defence.
- **Size:** the case holds up to **40 × 35 × 5 mm** (about 300–400 mAh), with
  only 0.2–0.3 mm to spare: wrap, leads and swelling count. The small-LiPo
  variant (`kucuk_pil` STLs) takes **30 × 37 × 4 mm**. Do not force a cell in.
- **Plug: JST PH 2.0 mm, 2-pin.** Not JST SH (1.0 mm) or the larger "JST RCY".

### Polarity: check before you plug it in

**JST PH plugs are not wired the same by every seller.** Adafruit and SparkFun
cells put red (+) on one side; many drone and RC cells put it on the other.
The board has **no reverse-polarity protection**: a reversed cell with POWER on
drives current backwards through the XIAO's charger. The cell gets hot and the
charger can be damaged (this happened on the first build).

On the board, J1 sits on the **back**. **Pin 1 = + (red)** is the pad next to
the small circle printed on the silkscreen; the other pad is − (black). The
plug goes in from the side of the keys.

Before the first connection:

1. Plug the cell in with **POWER off**.
2. Meter on DC volts: black probe on GND (the USB-C shell), red probe on the J1
   pad next to the circle.
3. You must read **+3.6 … +4.2 V**. A **negative** reading means the plug is
   reversed: unplug at once.

To fix a reversed plug, lift each crimp's tiny plastic latch with a needle,
pull the two wires out and swap them (tape one wire's end while you work on
the other so they cannot touch). Or cut the leads and fit a new plug.

If a cell got hot, unplug it, put it on a non-flammable surface and let it cool.
A cell that is swollen, smells, or was too hot to touch must not be used again:
take it to a battery recycling point.

### Switches

- **POWER** (SW2) connects the cell to the XIAO. **Off** = cell disconnected
  (storage, shipping). With USB plugged in, the board runs either way, but the
  cell **only charges with POWER on**.
- **MODE** (SW1) only chooses whether the USB cable is also a keyboard;
  Bluetooth works in both positions.

### Check the charger before the first cell

Do this once after soldering a XIAO, with **no cell plugged in** (or POWER
off): plug in USB and read the battery voltage, either with `make selftest`
(line *VBAT*) or with a meter on the XIAO side of SW2 against GND. It must be
about **4.1–4.2 V**: that is the charger's regulated output.

**Above 4.3 V (for example about 5 V) the charger is damaged** and passes USB
voltage straight to the cell. Do not connect a cell to that XIAO: it would be
over-charged (a protected cell then cuts itself off, an unprotected one can
fail). This is what a reversed cell did to the first build's charger. Use that
XIAO on USB only, or replace it.

### Charging

- Plug USB-C in with **POWER on**. The XIAO's BQ25101 charges the cell at
  **50 mA** (default; `CHARGE_100MA 1` in the firmware for 100 mA). Keep the
  current at or below about 1C of the cell: 50 mA is right for 50–300 mAh.
- The **CHG LED** (small, **red**, next to the USB-C connector, marked `CH`) is
  on while charging and off when full. With no cell, or POWER off, it stays off.
  The other LED (marked `USR`, RGB) is the firmware's status LED.
- A 300 mAh cell charges in about 6–7 h at 50 mA.

### Battery level

The level shows in the computer's Bluetooth settings (macOS: the battery icon
next to the device; Windows: Bluetooth devices).

- It is the cell **voltage** mapped to a percentage with a LiPo discharge curve
  (4.2 V = 100 %, 3.7 V ≈ 27 %, 3.3 V = 0 %), so it is a guide, not a gauge.
- **On USB** the reading is the charger's output, not the cell (it sits near
  4.2–5 V and moves with the charge current). The level is therefore held on
  USB: it can go up there but never down. The real level shows on battery.
- On battery it is updated every 30 s and sent to the computer when it changes
  by 2 % or more.
- If the percentage is clearly wrong on battery, measure the cell with a meter
  and adjust `VBAT_DIVIDER` in the firmware.

### Battery life

With deep sleep (after 10 min idle when connected, 3 min when not connected) a
300 mAh cell lasts **weeks to months** depending on use. Lift-to-wake on a
Sense keeps the motion sensor on while asleep, which costs a little more.
Wake it with any button (or, on a Sense, by picking it up).

### Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| Cell gets warm or hot | Reversed plug (see [Polarity](#polarity-check-before-you-plug-it-in)) or a short: unplug at once |
| Dead on battery, works on USB | POWER off; cell flat or its protection tripped; reversed plug; BAT pads under the XIAO not soldered |
| CHG LED never lights, cell does not charge | POWER off; cell full; charger damaged by a reversed cell |
| Level stuck at 100 % | Normal on USB; check on battery after a few minutes |
| Level drops while on USB | Old firmware; current firmware holds the level on USB |
| Serial *power* line shows VBAT around 5 V on USB | Charger damaged (often by a reversed cell): do not connect a cell, replace the XIAO |
| Cell reads above 4.25 V | Over-charged: stop using it, recycle it |
