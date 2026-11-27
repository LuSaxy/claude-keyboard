# Clawd self-test

A short program that checks whether the XIAO nRF52840 (or Plus) is soldered
correctly onto the Clawd V2 board, and lets you try every button. Flash it
after soldering (or reworking) the module, before the keyboard firmware.

```sh
arduino-cli compile --upload -p /dev/cu.usbmodem1101 \
  -b Seeeduino:nrf52:xiaonRF52840Plus .
```

Use `Seeeduino:nrf52:xiaonRF52840` for the plain XIAO (the Plus pads are then
skipped). Open a serial monitor at 115200 baud. The report repeats every 20 s,
or send `t` to run it at once, so you can watch it while reworking a joint.

## What it checks

1. **Levels with pull-up:** every used pin should read HIGH (about 3300 mV).
   D3 drives the eye LEDs and reads about 1.9 V; D2 shows the MODE position.
   A pin near 0 V is shorted to GND (or its button is held); a pin halfway is
   bridged to another pin.
2. **Tied to a supply:** no pin may read HIGH with a pull-down.
3. **Bridges:** each pin in turn is driven HIGH and every other pin that
   follows it is listed. "D3 is joined to: D4 D5" means a solder bridge
   between those pads. Pins already held LOW are not driven.
4. **Battery:** the voltage on the battery divider. The real check is to
   switch POWER on, unplug USB and see the eyes keep blinking.

After **RESULT: PASS** the eyes blink once a second. Press B1, B2, L1, R1 and
flip MODE: every change is printed and the green LED lights while a button is
down. Then flash `../xiao_ble_keyboard` again.

## XIAO nRF52840 Plus on the V2 board

The V2 board has pads for the standard XIAO (14 castellations, BAT+/BAT−).
The Plus has nine extra 1.27 mm pads **between** the standard ones: D11–D16
between D0–D6 and D17–D19 between D10–D7. When the module is soldered with
hot air or a hot plate, the solder on the board pads flows onto these extra
pads and joins the neighbouring pins (measured on the first build: D3, D4,
D5, D6 and the extra pads D13–D16 joined, D12 joined to D2).

With a Plus on this board:

- Put **Kapton tape** (or solder mask paint) over the nine extra pads on the
  underside of the module before soldering, or
- solder the standard castellations from the side with an iron instead of
  reflowing the whole module, using little solder, and
- run this self-test before closing the case.

A plain **XIAO nRF52840** (not Plus) has no extra pads and avoids the problem.
