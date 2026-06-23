<script setup>
// Clawd Web Bluetooth control panel, ported to a Vue component.
// All Bluetooth access happens in event handlers / onMounted, so it is safe to
// render under <ClientOnly>. Works in Chrome/Edge over https:// or localhost.
import { computed, onMounted, reactive, ref } from "vue";

const UUID = {
  ledService: "494dceab-1533-4012-afaa-20d5acfc6ecf",
  led: "bc6df096-9c5a-4034-89d2-8440c14d2ab0",
  configService: "e0934030-b408-4dcc-b4ca-56d83d7c5fbc",
  keymap: "e0c5d214-8809-4243-afa2-9862d3628c38",
  name: "8adf91b2-c9a1-4c1a-8b76-2ff5be6b86de",
  command: "37508895-a542-4ee0-9ab4-b109ce4717de",
};

const CMD_RESET_KEYMAP = 0x01;
const CMD_RESET_NAME = 0x02;

// Full HID Keyboard/Keypad usage page, grouped for the dropdown. Codes are the
// standard usage IDs; the firmware accepts any of them (keyboard codes are a
// single byte, so 0x00-0xFF). See program/.../xiao_ble_keyboard.ino.
const keyboardGroups = [
  ["Letters", [
    ["A", 0x04], ["B", 0x05], ["C", 0x06], ["D", 0x07], ["E", 0x08],
    ["F", 0x09], ["G", 0x0a], ["H", 0x0b], ["I", 0x0c], ["J", 0x0d],
    ["K", 0x0e], ["L", 0x0f], ["M", 0x10], ["N", 0x11], ["O", 0x12],
    ["P", 0x13], ["Q", 0x14], ["R", 0x15], ["S", 0x16], ["T", 0x17],
    ["U", 0x18], ["V", 0x19], ["W", 0x1a], ["X", 0x1b], ["Y", 0x1c],
    ["Z", 0x1d],
  ]],
  ["Numbers", [
    ["1", 0x1e], ["2", 0x1f], ["3", 0x20], ["4", 0x21], ["5", 0x22],
    ["6", 0x23], ["7", 0x24], ["8", 0x25], ["9", 0x26], ["0", 0x27],
  ]],
  ["Basic", [
    ["Enter", 0x28], ["Escape", 0x29], ["Backspace", 0x2a], ["Tab", 0x2b],
    ["Space", 0x2c], ["Caps Lock", 0x39], ["Menu", 0x65],
  ]],
  ["Symbols", [
    ["- _", 0x2d], ["= +", 0x2e], ["[ {", 0x2f], ["] }", 0x30], ["\\ |", 0x31],
    ["; :", 0x33], ["' \"", 0x34], ["` ~", 0x35], [", <", 0x36], [". >", 0x37],
    ["/ ?", 0x38],
  ]],
  ["Navigation", [
    ["Insert", 0x49], ["Home", 0x4a], ["Page Up", 0x4b], ["Delete", 0x4c],
    ["End", 0x4d], ["Page Down", 0x4e], ["Arrow Right", 0x4f], ["Arrow Left", 0x50],
    ["Arrow Down", 0x51], ["Arrow Up", 0x52], ["Print Screen", 0x46],
    ["Scroll Lock", 0x47], ["Pause", 0x48],
  ]],
  ["Function", [
    ["F1", 0x3a], ["F2", 0x3b], ["F3", 0x3c], ["F4", 0x3d], ["F5", 0x3e],
    ["F6", 0x3f], ["F7", 0x40], ["F8", 0x41], ["F9", 0x42], ["F10", 0x43],
    ["F11", 0x44], ["F12", 0x45], ["F13", 0x68], ["F14", 0x69], ["F15", 0x6a],
    ["F16", 0x6b], ["F17", 0x6c], ["F18", 0x6d], ["F19", 0x6e], ["F20", 0x6f],
    ["F21", 0x70], ["F22", 0x71], ["F23", 0x72], ["F24", 0x73],
  ]],
  ["Keypad", [
    ["Num Lock", 0x53], ["Keypad /", 0x54], ["Keypad *", 0x55], ["Keypad -", 0x56],
    ["Keypad +", 0x57], ["Keypad Enter", 0x58], ["Keypad 1", 0x59], ["Keypad 2", 0x5a],
    ["Keypad 3", 0x5b], ["Keypad 4", 0x5c], ["Keypad 5", 0x5d], ["Keypad 6", 0x5e],
    ["Keypad 7", 0x5f], ["Keypad 8", 0x60], ["Keypad 9", 0x61], ["Keypad 0", 0x62],
    ["Keypad .", 0x63],
  ]],
];

const mediaGroups = [
  ["Media", [
    ["Volume Up", 0x00e9], ["Volume Down", 0x00ea], ["Mute", 0x00e2],
    ["Play / Pause", 0x00cd], ["Stop", 0x00b7], ["Next Track", 0x00b5],
    ["Previous Track", 0x00b6],
  ]],
];

// Sentinel select value: "enter a raw HID code by hand".
const CUSTOM = -1;

// Flat lookups derived from the grouped lists.
const keyboardKeys = keyboardGroups.flatMap(([, items]) => items);
const mediaKeys = mediaGroups.flatMap(([, items]) => items);

function groupsFor(type) {
  return type === TYPE_MEDIA ? mediaGroups : keyboardGroups;
}
function baseFor(type) {
  return type === TYPE_MEDIA ? mediaKeys : keyboardKeys;
}
function inBase(type, code) {
  return baseFor(type).some(([, value]) => value === code);
}
function toHex(code) {
  return `0x${Number(code).toString(16).toUpperCase().padStart(2, "0")}`;
}

const TYPE_KEYBOARD = 0;
const TYPE_MEDIA = 1;
const MOD_CTRL = 0x01;
const MOD_SHIFT = 0x02;
const MOD_ALT = 0x04;
const MOD_GUI = 0x08;

const encoder = new TextEncoder();
const decoder = new TextDecoder();
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// Non-reactive connection handles.
let device = null;
let chars = {};
// Devices this site already has permission for (Web Bluetooth getDevices()).
let knownMap = new Map();

// Reactive UI state.
const connected = ref(false);
const supported = ref(true);
const statusKind = ref(""); // '', 'ready', 'error'
const logText = ref("Ready. Press CONNECT.");
const logKind = ref("");
const name = ref("");
const led = ref(0);
const knownDevices = ref([]); // [{ id, name, active }] from getDevices()

const b1 = reactive(makeButton(0x68));
const b2 = reactive(makeButton(0x69));

function makeButton(code) {
  return {
    type: TYPE_KEYBOARD,
    code,
    sel: code, // dropdown selection: a real code, or CUSTOM
    custom: false, // raw-code input mode active?
    customInput: "", // text in the custom-code field
    error: "", // validation message for the custom field
    ctrl: false,
    shift: false,
    alt: false,
    gui: false,
  };
}

const badgeText = computed(() =>
  statusKind.value === "error"
    ? "ERROR"
    : connected.value
      ? "CONNECTED"
      : "DISCONNECTED",
);
const badgeClass = computed(() =>
  statusKind.value === "error" ? "error" : connected.value ? "ready" : "",
);

function setStatus(message, kind = "") {
  logText.value = message;
  logKind.value = kind === "ready" ? "ready" : kind === "error" ? "error" : "";
  statusKind.value = kind === "error" ? "error" : "";
}

function run(promise, okMessage) {
  return promise
    .then((result) => {
      if (okMessage) setStatus(okMessage, "ready");
      return result;
    })
    .catch((error) =>
      setStatus(String(error && error.message ? error.message : error), "error"),
    );
}

// ----- keymap option handling -----

// Parse the custom-code field into button.code, validating against the range
// the firmware accepts (keyboard codes are one byte; consumer codes are 16-bit).
function parseCustom(button) {
  const raw = String(button.customInput).trim();
  if (!raw) {
    button.error = "Enter a HID code.";
    return;
  }
  let value;
  if (/^0x[0-9a-f]+$/i.test(raw)) value = parseInt(raw, 16);
  else if (/^[0-9]+$/.test(raw)) value = parseInt(raw, 10);
  else {
    button.error = "Use hex (0x4F) or a decimal number.";
    return;
  }
  const max = button.type === TYPE_MEDIA ? 0xffff : 0xff;
  if (!Number.isFinite(value) || value < 0 || value > max) {
    button.error = `Out of range (0–${toHex(max)}).`;
    return;
  }
  button.error = "";
  button.code = value;
}

// React to a dropdown change: either a listed key, or switch to custom input.
function onSelChange(button) {
  if (button.sel === CUSTOM) {
    button.custom = true;
    if (!button.customInput) button.customInput = toHex(button.code);
    parseCustom(button);
  } else {
    button.custom = false;
    button.error = "";
    button.code = button.sel;
  }
}

function onTypeChange(button) {
  if (button.type === TYPE_MEDIA) {
    button.ctrl = button.shift = button.alt = button.gui = false;
  }
  // Reset to the first key of the new type and leave custom mode.
  const first = baseFor(button.type)[0][1];
  button.custom = false;
  button.customInput = "";
  button.error = "";
  button.sel = first;
  button.code = first;
}

function setButtonUi(button, type, mod, code) {
  button.type = type;
  button.code = code;
  button.error = "";
  if (inBase(type, code)) {
    button.custom = false;
    button.customInput = "";
    button.sel = code;
  } else {
    // Unknown code stored on the device: expose it in the custom field.
    button.custom = true;
    button.customInput = toHex(code);
    button.sel = CUSTOM;
  }
  button.ctrl = Boolean(mod & MOD_CTRL);
  button.shift = Boolean(mod & MOD_SHIFT);
  button.alt = Boolean(mod & MOD_ALT);
  button.gui = Boolean(mod & MOD_GUI);
}

function readButton(button) {
  const mod =
    button.type === TYPE_MEDIA
      ? 0
      : (button.ctrl ? MOD_CTRL : 0) |
        (button.shift ? MOD_SHIFT : 0) |
        (button.alt ? MOD_ALT : 0) |
        (button.gui ? MOD_GUI : 0);
  return { type: button.type, mod, code: button.code };
}

function encodeKeymap() {
  const a = readButton(b1);
  const b = readButton(b2);
  return new Uint8Array([
    a.type, a.mod, a.code & 0xff, a.code >> 8,
    b.type, b.mod, b.code & 0xff, b.code >> 8,
  ]);
}

function applyState(state) {
  name.value = state.name || "";
  led.value = state.led;
  const k = state.keymap;
  if (k && k.length === 8) {
    setButtonUi(b1, k[0], k[1], k[2] | (k[3] << 8));
    setButtonUi(b2, k[4], k[5], k[6] | (k[7] << 8));
  }
}

// ----- GATT helpers -----

async function readRetry(characteristic) {
  const deadline = Date.now() + 30000;
  let lastError;
  let prompted = false;
  for (;;) {
    try {
      return await characteristic.readValue();
    } catch (error) {
      lastError = error;
    }
    if (!prompted) {
      setStatus(
        "Waiting for pairing...\n\nIf the device LED is MAGENTA, press Button 1 " +
          "on the device to confirm pairing. This can take up to ~30s.",
      );
      prompted = true;
    }
    if (Date.now() >= deadline) {
      throw new Error(
        `Could not read from Clawd (${lastError}).\n\n` +
          "If the device LED is MAGENTA, press Button 1 to confirm pairing. " +
          "If it was paired before with old firmware, remove 'clawd' in your OS " +
          "Bluetooth settings and bond-reset the device (hold both buttons at boot), " +
          "then reconnect.",
      );
    }
    await sleep(800);
  }
}

async function writeValue(characteristic, data) {
  if (characteristic.writeValueWithResponse) {
    await characteristic.writeValueWithResponse(data);
  } else {
    await characteristic.writeValue(data);
  }
}

async function loadState() {
  const ledByte = (await readRetry(chars.led)).getUint8(0);
  const nameView = await readRetry(chars.name);
  const deviceName = decoder.decode(nameView).replace(/\0+$/, "");
  const keymapView = await readRetry(chars.keymap);
  const keymap = Array.from(new Uint8Array(keymapView.buffer));
  applyState({ led: ledByte, name: deviceName, keymap });
}

// ----- actions -----

function onDisconnected() {
  connected.value = false;
  setStatus("Device disconnected.");
  refreshKnownDevices();
}

// Pull the devices this site has already been granted access to, so the user can
// reconnect to a known Clawd without opening the browser chooser again.
// getDevices() is Chrome/Edge only and may require the
// chrome://flags/#enable-web-bluetooth-new-permissions-backend flag.
async function refreshKnownDevices() {
  if (!navigator.bluetooth || !navigator.bluetooth.getDevices) {
    return;
  }
  try {
    const list = await navigator.bluetooth.getDevices();
    knownMap = new Map(list.map((d) => [d.id, d]));
    knownDevices.value = list.map((d) => ({
      id: d.id,
      name: d.name || "Unnamed device",
      active: !!(d.gatt && d.gatt.connected),
    }));
  } catch {
    knownDevices.value = [];
  }
}

// Shared GATT setup used by both the chooser flow and the known-device list.
async function connectToDevice(target) {
  device = target;
  device.removeEventListener("gattserverdisconnected", onDisconnected);
  device.addEventListener("gattserverdisconnected", onDisconnected);

  setStatus(`Connecting to ${device.name || "Clawd"}...`);
  const server = await device.gatt.connect();
  const ledService = await server.getPrimaryService(UUID.ledService);
  const configService = await server.getPrimaryService(UUID.configService);
  chars = {
    led: await ledService.getCharacteristic(UUID.led),
    keymap: await configService.getCharacteristic(UUID.keymap),
    name: await configService.getCharacteristic(UUID.name),
    command: await configService.getCharacteristic(UUID.command),
  };

  await loadState();
  connected.value = true;
  setStatus(`Connected to ${device.name || "Clawd"}.`, "ready");
  await refreshKnownDevices();
}

async function connect() {
  if (!navigator.bluetooth) {
    setStatus(
      "Web Bluetooth is not available. Use Chrome or Edge, served over https:// " +
        "or http://localhost (not file://).",
      "error",
    );
    return;
  }

  setStatus("Opening the browser device chooser... pick your Clawd.");
  const chosen = await navigator.bluetooth.requestDevice({
    acceptAllDevices: true,
    optionalServices: [UUID.ledService, UUID.configService],
  });
  await connectToDevice(chosen);
}

// Reconnect to a device from the known-device list, no chooser needed.
async function connectKnown(id) {
  const target = knownMap.get(id);
  if (!target) {
    throw new Error("That device is no longer available. Use CONNECT.");
  }
  await connectToDevice(target);
}

function disconnect() {
  if (device && device.gatt && device.gatt.connected) {
    device.gatt.disconnect();
  }
  connected.value = false;
  setStatus("Disconnected.");
  refreshKnownDevices();
}

function requireConnection() {
  if (!connected.value || !chars.led) {
    throw new Error("Connect to Clawd first.");
  }
}

async function setLed(level) {
  requireConnection();
  const value = Number(level);
  await writeValue(chars.led, new Uint8Array([value]));
  led.value = value;
}

async function saveName() {
  requireConnection();
  const value = name.value.trim();
  const bytes = encoder.encode(value);
  if (bytes.length === 0 || bytes.length > 15) {
    throw new Error("Name must be 1-15 ASCII bytes.");
  }
  if ([...bytes].some((byte) => byte < 32 || byte > 126)) {
    throw new Error("Name must be printable ASCII.");
  }
  await writeValue(chars.name, bytes);
}

async function saveKeys() {
  requireConnection();
  if (b1.error || b2.error) {
    throw new Error("Fix the custom key code first.");
  }
  await writeValue(chars.keymap, encodeKeymap());
}

async function sendCommand(command) {
  requireConnection();
  await writeValue(chars.command, new Uint8Array([command]));
  await sleep(150);
  await loadState();
}

onMounted(() => {
  if (!navigator.bluetooth) {
    supported.value = false;
    setStatus(
      "Web Bluetooth is not available in this browser. Use Chrome or Edge over " +
        "https:// or http://localhost.",
      "error",
    );
    return;
  }
  refreshKnownDevices();
});
</script>

<template>
  <div class="clawd">
    <header class="bar">
      <div class="brand">
        <span class="logo-dot" aria-hidden="true"></span>
        <span class="mark">Clawd</span>
        <span class="sub">BLE Control</span>
      </div>
      <span class="status" :class="badgeClass">{{ badgeText }}</span>
    </header>

    <section class="panel">
      <h3>Device</h3>
      <p class="hint">
        Pair <b>clawd</b> in your OS Bluetooth settings first, then connect.
        Chrome / Edge only.
      </p>
      <ul v-if="knownDevices.length" class="known">
        <li v-for="d in knownDevices" :key="d.id">
          <button
            class="known-item"
            :class="{ active: d.active }"
            :disabled="connected"
            @click="run(connectKnown(d.id))"
          >
            <span class="known-dot" aria-hidden="true"></span>
            <span class="known-name">{{ d.name }}</span>
            <span class="known-go">{{ d.active ? "LINKED" : "CONNECT" }}</span>
          </button>
        </li>
      </ul>
      <div class="ctl">
        <button class="btn primary" :disabled="connected" @click="run(connect())">
          {{ knownDevices.length ? "PAIR NEW" : "CONNECT" }}
        </button>
        <button class="btn" :disabled="!connected" @click="disconnect()">DROP</button>
      </div>
    </section>

    <section class="panel" :class="{ disabled: !connected }">
      <h3>Name</h3>
      <div class="ctl">
        <input v-model="name" maxlength="15" autocomplete="off" placeholder="clawd" />
        <button class="btn" @click="run(saveName(), 'Name saved. Reconnect for BLE; reboot for USB.')">
          SET
        </button>
        <button class="btn" @click="run(sendCommand(CMD_RESET_NAME), 'Name reset to clawd.')">
          RST
        </button>
      </div>
    </section>

    <section class="panel" :class="{ disabled: !connected }">
      <h3>LED <span class="val">{{ led }}</span></h3>
      <input
        type="range"
        min="0"
        max="255"
        v-model.number="led"
        @change="run(setLed(led), 'LED updated.')"
      />
      <div class="grid4">
        <button class="btn" @click="run(setLed(0), 'LED updated.')">OFF</button>
        <button class="btn" @click="run(setLed(64), 'LED updated.')">LOW</button>
        <button class="btn" @click="run(setLed(128), 'LED updated.')">MID</button>
        <button class="btn" @click="run(setLed(255), 'LED updated.')">MAX</button>
      </div>
    </section>

    <section class="panel" :class="{ disabled: !connected }">
      <h3>Buttons</h3>
      <div class="keys">
        <article v-for="(button, i) in [b1, b2]" :key="i" class="key">
          <span class="tag">B{{ i + 1 }}</span>
          <select v-model.number="button.type" @change="onTypeChange(button)">
            <option :value="TYPE_KEYBOARD">Keyboard key</option>
            <option :value="TYPE_MEDIA">Media key</option>
          </select>
          <select v-model.number="button.sel" @change="onSelChange(button)">
            <optgroup v-for="[group, items] in groupsFor(button.type)" :key="group" :label="group">
              <option v-for="[label, value] in items" :key="value" :value="value">
                {{ label }}
              </option>
            </optgroup>
            <option :value="CUSTOM">Custom…</option>
          </select>
          <template v-if="button.custom">
            <input
              class="custom-code"
              v-model="button.customInput"
              @input="parseCustom(button)"
              placeholder="e.g. 0x2F or 47"
              autocomplete="off"
              spellcheck="false"
            />
            <span v-if="button.error" class="custom-msg error">{{ button.error }}</span>
            <span v-else class="custom-msg">Sends HID {{ toHex(button.code) }}</span>
          </template>
          <div class="mods" :class="{ disabled: button.type === TYPE_MEDIA }">
            <label><input type="checkbox" v-model="button.ctrl" :disabled="button.type === TYPE_MEDIA" />CTRL</label>
            <label><input type="checkbox" v-model="button.shift" :disabled="button.type === TYPE_MEDIA" />SHFT</label>
            <label><input type="checkbox" v-model="button.alt" :disabled="button.type === TYPE_MEDIA" />ALT</label>
            <label><input type="checkbox" v-model="button.gui" :disabled="button.type === TYPE_MEDIA" />GUI</label>
          </div>
        </article>
      </div>
      <div class="ctl">
        <button class="btn primary" @click="run(saveKeys(), 'Button mapping saved.')">SAVE</button>
        <button class="btn" @click="run(sendCommand(CMD_RESET_KEYMAP), 'Buttons reset to F13 / F14.')">
          RESET F13/F14
        </button>
      </div>
    </section>

    <section class="panel">
      <h3>Log</h3>
      <pre class="log" :class="logKind">{{ logText }}</pre>
    </section>
  </div>
</template>

<style scoped>
.clawd {
  width: min(100%, 620px);
  margin: 24px auto 8px;
  display: flex;
  flex-direction: column;
  gap: 14px;
  font-size: 14px;
}

.bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--vp-c-divider);
}

.brand {
  display: flex;
  align-items: center;
  gap: 10px;
}

.logo-dot {
  width: 14px;
  height: 14px;
  border-radius: 5px;
  background: var(--vp-c-brand-3);
}

.mark {
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.01em;
}

.sub {
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--vp-c-text-2);
}

.status {
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  border: 1px solid var(--vp-c-border);
  border-radius: 999px;
  padding: 5px 12px;
  color: var(--vp-c-text-2);
  background: var(--vp-c-bg-elv);
  white-space: nowrap;
}

.status.ready {
  color: #fff;
  background: #2f8f5b;
  border-color: transparent;
}

.status.error {
  color: #fff;
  background: #c0392b;
  border-color: transparent;
}

.panel {
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-divider);
  border-radius: 14px;
  padding: 16px;
}

.panel.disabled {
  opacity: 0.55;
  pointer-events: none;
}

.panel h3 {
  margin: 0 0 12px;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--vp-c-text-2);
  display: flex;
  justify-content: space-between;
  align-items: center;
  border: 0;
  padding: 0;
}

.val {
  color: var(--vp-c-brand-1);
  font-weight: 700;
}

.hint {
  margin: 0 0 12px;
  font-size: 13px;
  color: var(--vp-c-text-2);
}

.hint b {
  color: var(--vp-c-text-1);
}

.ctl {
  display: flex;
  gap: 10px;
}

.known {
  list-style: none;
  margin: 0 0 12px;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.known-item {
  display: flex;
  align-items: center;
  gap: 10px;
  width: 100%;
  min-height: 44px;
  padding: 0 14px;
  text-align: left;
  font: inherit;
  font-size: 13px;
  font-weight: 600;
  color: var(--vp-c-text-1);
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-border);
  border-radius: 10px;
  cursor: pointer;
  transition: border-color 0.15s ease, color 0.15s ease;
}

.known-item:hover:not(:disabled) {
  border-color: var(--vp-c-brand-3);
  color: var(--vp-c-brand-1);
}

.known-item:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.known-dot {
  flex: none;
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--vp-c-gutter);
}

.known-item.active .known-dot {
  background: #2f8f5b;
}

.known-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.known-go {
  flex: none;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.12em;
  color: var(--vp-c-text-2);
}

input,
select {
  font: inherit;
  color: var(--vp-c-text-1);
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-border);
  border-radius: 10px;
  min-height: 40px;
  padding: 0 12px;
  width: 100%;
  min-width: 0;
}

input:focus-visible,
select:focus-visible {
  outline: 2px solid var(--vp-c-brand-3);
  outline-offset: 1px;
}

input[type="range"] {
  appearance: none;
  -webkit-appearance: none;
  height: 8px;
  min-height: 0;
  padding: 0;
  border: none;
  border-radius: 999px;
  background: var(--vp-c-gutter);
}

input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: var(--vp-c-brand-3);
  border: 2px solid var(--vp-c-bg-elv);
  cursor: pointer;
}

input[type="range"]::-moz-range-thumb {
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: var(--vp-c-brand-3);
  border: 2px solid var(--vp-c-bg-elv);
  cursor: pointer;
}

.btn {
  flex: 1;
  min-height: 40px;
  padding: 0 14px;
  font-weight: 600;
  font-size: 13px;
  letter-spacing: 0.02em;
  color: var(--vp-c-text-1);
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-border);
  border-radius: 999px;
  cursor: pointer;
  transition: border-color 0.15s ease, color 0.15s ease, background 0.15s ease;
}

.btn:hover:not(:disabled) {
  border-color: var(--vp-c-brand-3);
  color: var(--vp-c-brand-1);
}

.btn.primary {
  background: var(--vp-c-brand-3);
  border-color: var(--vp-c-brand-3);
  color: #fff;
}

.btn.primary:hover:not(:disabled) {
  background: var(--vp-c-brand-2);
  border-color: var(--vp-c-brand-2);
  color: #fff;
}

.btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.grid4 {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 10px;
  margin-top: 12px;
}

.keys {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-bottom: 12px;
}

.key {
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-divider);
  border-radius: 10px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.tag {
  font-size: 11px;
  font-weight: 800;
  letter-spacing: 0.12em;
  color: var(--vp-c-brand-1);
}

.custom-code {
  font-family: var(--vp-font-family-mono);
  font-size: 13px;
}

.custom-msg {
  font-size: 11px;
  color: var(--vp-c-text-2);
}

.custom-msg.error {
  color: #c0392b;
}

.mods {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 6px;
}

.mods.disabled {
  opacity: 0.4;
}

.mods label {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 11px;
  font-weight: 500;
  border: 1px solid var(--vp-c-border);
  border-radius: 8px;
  background: var(--vp-c-bg-elv);
  padding: 6px 8px;
  cursor: pointer;
}

.mods input {
  appearance: none;
  -webkit-appearance: none;
  width: 14px;
  height: 14px;
  min-height: 0;
  padding: 0;
  border: 1px solid var(--vp-c-border);
  border-radius: 4px;
  background: var(--vp-c-bg);
  cursor: pointer;
}

.mods input:checked {
  background: var(--vp-c-brand-3);
  border-color: var(--vp-c-brand-3);
}

.log {
  margin: 0;
  font-family: var(--vp-font-family-mono);
  font-size: 12px;
  line-height: 1.5;
  color: var(--vp-c-text-2);
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-divider);
  border-radius: 10px;
  padding: 12px;
  white-space: pre-wrap;
  word-break: break-word;
  overflow-wrap: anywhere;
  max-height: 260px;
  min-height: 64px;
  overflow-y: auto;
}

.log.ready {
  color: #2f8f5b;
}

.log.error {
  color: #c0392b;
}

@media (max-width: 460px) {
  .keys {
    grid-template-columns: 1fr;
  }
}
</style>
