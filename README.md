# Clawd Keyboard

![Clawd keyboard preview](models/view.jpeg)

**Clawd Keyboard** is a small two-button keyboard built around the Seeed Studio XIAO
nRF52840. It can type over Bluetooth LE or USB-HID, run from a LiPo battery, and
sleep deeply when it is not in use.

It is meant to be a tiny, customisable shortcut pad: map the buttons to free
keys like `F13`/`F14`, media controls, or your own app shortcuts.

## Highlights

- Two-button BLE + USB keyboard firmware for XIAO nRF52840.
- Default keys are `F13` and `F14`, so they are easy to bind in other tools.
- Runtime BLE configuration for key mappings, device name, and the external LED.
- Confirmed BLE pairing, battery reporting, LiPo charging, and deep sleep.
- Cross-platform desktop control app (Tauri + native Rust BLE).
- Printable mascot-style enclosure and FreeCAD sources in `models/`.

## Repository

| Path | Contents |
| --- | --- |
| `program/xiao_ble_keyboard/` | Arduino firmware and detailed build/flash notes. |
| `desktop/` | Tauri + Rust desktop app for LED, keymap, and device name changes. |
| `models/` | FreeCAD sources, printable 3MF files, drawing export, and preview image. |

## Models

The current model assets are:

- `models/view.jpeg` - preview image used above.
- `models/mascot-top.3mf` and `models/mascot-bottom.3mf` - printable enclosure parts.
- `models/assembly.FCStd` and `models/mascot.FCStd` - FreeCAD source files.
- `models/assembly_Page__.svg` - exported drawing page.

More build photos and renders can be dropped into `models/` as the project grows.

## Firmware

Start with the firmware README:

[`program/xiao_ble_keyboard/README.md`](program/xiao_ble_keyboard/README.md)

It covers wiring, pairing, custom key mappings, battery behaviour, Arduino IDE
setup, and upload troubleshooting.

## Desktop App

The `desktop/` folder contains a cross-platform control app built with
**Tauri v2 + Rust**. Bluetooth is handled natively in Rust (`btleplug`) — no Web
Bluetooth. After pairing with **clawd**, it can:

- set the external LED brightness;
- remap Button 1 and Button 2;
- change the BLE/USB device name;
- reset the keymap or name to firmware defaults.

Runs on Windows, macOS, and Linux. See [`desktop/README.md`](desktop/README.md).

## Git LFS

Files under `models/` are configured for Git LFS via `.gitattributes`, so large
CAD files, 3D-print files, renders, and photos stay out of normal Git history.

Before adding model files on a new machine, install Git LFS and run:

```sh
git lfs install
```
