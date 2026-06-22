# Clawd Control (Web)

A small Web Bluetooth control panel for Clawd. Runs in the browser — no install.

It can:

- set the external LED brightness;
- remap Button 1 and Button 2 (keyboard or media keys);
- change the BLE/USB device name;
- reset the keymap or name to firmware defaults.

## Requirements

- **Chrome or Edge** (desktop or Android). Web Bluetooth is not in Firefox/Safari.
- A **secure context**: `https://` or `http://localhost`. `file://` will not work.

## Run

Serve this folder over localhost, then open it in Chrome/Edge:

```sh
# from the repo root
python3 -m http.server 8000
```

Open `http://localhost:8000/app/`.

## Use

1. Pair **clawd** once in your OS Bluetooth settings (Windows/macOS/Linux). When
   the device LED turns **magenta**, press **Button 1** to confirm.
2. In the page, press **CONNECT** and pick your device in the browser chooser.
3. The first read may trigger pairing again — if the LED is magenta, press
   **Button 1**. Then the LED, name, and button settings load.

If you previously paired with old firmware and reads fail, remove `clawd` in your
OS Bluetooth settings and bond-reset the device (hold both buttons at boot), then
reconnect.

## Notes

- Media keys are sent over BLE only (the USB-HID interface is keyboard-only).
- A new device name shows over BLE after reconnect, and over USB after a reboot.
