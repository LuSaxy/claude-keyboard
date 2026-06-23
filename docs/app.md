---
title: Control app
aside: false
pageClass: clawd-app-page
---

# Control app

A small **Web Bluetooth** control panel for Clawd — runs right here in the
browser, no install. After pairing with **clawd**, it can set the external LED
brightness, remap Button 1 and Button 2 (keyboard or media keys), change the
BLE/USB device name, and reset the keymap or name to firmware defaults.

::: info Requirements
**Chrome or Edge** (desktop or Android) over `https://` — Web Bluetooth is not in
Firefox/Safari. Pair **clawd** in your OS Bluetooth settings first; when the
device LED turns **magenta**, press **Button 1** to confirm.
:::

<ClientOnly>
  <ClawdApp />
</ClientOnly>
