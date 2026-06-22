// Clawd Web Bluetooth control panel.
// Runs in Chrome/Edge over https:// or http://localhost (secure context).

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

const keyboardKeys = [
  ["F13", 0x68],
  ["F14", 0x69],
  ["F15", 0x6a],
  ["F16", 0x6b],
  ["A", 0x04],
  ["B", 0x05],
  ["C", 0x06],
  ["Enter", 0x28],
  ["Escape", 0x29],
  ["Tab", 0x2b],
  ["Space", 0x2c],
  ["Arrow Right", 0x4f],
  ["Arrow Left", 0x50],
  ["Arrow Down", 0x51],
  ["Arrow Up", 0x52],
  ["Delete", 0x4c],
];

const mediaKeys = [
  ["Volume Up", 0x00e9],
  ["Volume Down", 0x00ea],
  ["Mute", 0x00e2],
  ["Play / Pause", 0x00cd],
  ["Next Track", 0x00b5],
  ["Previous Track", 0x00b6],
];

const TYPE_KEYBOARD = 0;
const TYPE_MEDIA = 1;
const MOD_CTRL = 0x01;
const MOD_SHIFT = 0x02;
const MOD_ALT = 0x04;
const MOD_GUI = 0x08;

const $ = (id) => document.getElementById(id);
const encoder = new TextEncoder();
const decoder = new TextDecoder();
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let device = null;
let chars = {};
let connected = false;

// ----- status / log -----

function paintBadge(kind) {
  const el = $("status");
  el.textContent = kind === "error" ? "ERROR" : connected ? "CONNECTED" : "DISCONNECTED";
  el.classList.toggle("ready", kind !== "error" && connected);
  el.classList.toggle("error", kind === "error");
}

function setStatus(message, kind = "") {
  const logEl = $("log");
  logEl.textContent = message;
  logEl.classList.toggle("ready", kind === "ready");
  logEl.classList.toggle("error", kind === "error");
  logEl.scrollTop = 0;
  paintBadge(kind);
}

function setConnected(state) {
  connected = state;
  $("disconnect").disabled = !state;
  for (const card of document.querySelectorAll("[data-requires-connection]")) {
    card.setAttribute("aria-disabled", String(!state));
  }
  paintBadge("");
}

function run(promise, okMessage) {
  promise
    .then((result) => {
      if (okMessage) setStatus(okMessage, "ready");
      return result;
    })
    .catch((error) => setStatus(String(error && error.message ? error.message : error), "error"));
}

// ----- buttons / keymap UI -----

function populateType(select) {
  select.replaceChildren(
    new Option("Keyboard key", String(TYPE_KEYBOARD)),
    new Option("Media key", String(TYPE_MEDIA)),
  );
}

function populateCodes(prefix) {
  const type = Number($(`${prefix}-type`).value);
  const select = $(`${prefix}-code`);
  const keys = type === TYPE_MEDIA ? mediaKeys : keyboardKeys;
  select.replaceChildren(...keys.map(([label, value]) => new Option(label, String(value))));
  updateModifierState(prefix);
}

function updateModifierState(prefix) {
  const media = Number($(`${prefix}-type`).value) === TYPE_MEDIA;
  $(`${prefix}-mods`).classList.toggle("disabled", media);
  for (const id of ["ctrl", "shift", "alt", "gui"]) {
    $(`${prefix}-${id}`).disabled = media;
  }
}

function setButtonUi(prefix, type, mod, code) {
  $(`${prefix}-type`).value = String(type);
  populateCodes(prefix);
  const codeSelect = $(`${prefix}-code`);
  const value = String(code);
  if (![...codeSelect.options].some((option) => option.value === value)) {
    const hex = `0x${code.toString(16).toUpperCase().padStart(2, "0")}`;
    codeSelect.append(new Option(`Custom ${hex}`, value));
  }
  codeSelect.value = value;
  $(`${prefix}-ctrl`).checked = Boolean(mod & MOD_CTRL);
  $(`${prefix}-shift`).checked = Boolean(mod & MOD_SHIFT);
  $(`${prefix}-alt`).checked = Boolean(mod & MOD_ALT);
  $(`${prefix}-gui`).checked = Boolean(mod & MOD_GUI);
  updateModifierState(prefix);
}

function readButton(prefix) {
  const type = Number($(`${prefix}-type`).value);
  const code = Number($(`${prefix}-code`).value);
  const mod = type === TYPE_MEDIA
    ? 0
    : ($(`${prefix}-ctrl`).checked ? MOD_CTRL : 0)
      | ($(`${prefix}-shift`).checked ? MOD_SHIFT : 0)
      | ($(`${prefix}-alt`).checked ? MOD_ALT : 0)
      | ($(`${prefix}-gui`).checked ? MOD_GUI : 0);
  return { type, mod, code };
}

function encodeKeymap() {
  const b1 = readButton("b1");
  const b2 = readButton("b2");
  return new Uint8Array([
    b1.type, b1.mod, b1.code & 0xff, b1.code >> 8,
    b2.type, b2.mod, b2.code & 0xff, b2.code >> 8,
  ]);
}

function applyState(state) {
  $("device-name").value = state.name || "";
  $("led-slider").value = String(state.led);
  $("led-value").textContent = String(state.led);
  const k = state.keymap;
  if (k && k.length === 8) {
    setButtonUi("b1", k[0], k[1], k[2] | (k[3] << 8));
    setButtonUi("b2", k[4], k[5], k[6] | (k[7] << 8));
  }
}

// ----- GATT helpers -----

// Encrypted characteristics need a bonded link. The first read triggers the OS
// pairing; the device then shows a MAGENTA LED and you confirm with Button 1.
// That can take a while, so retry for ~30s.
async function readRetry(characteristic) {
  const deadline = Date.now() + 30000;
  let lastError;
  let promptedForPairing = false;
  for (;;) {
    try {
      return await characteristic.readValue();
    } catch (error) {
      lastError = error;
    }
    // We only get here when a read failed, which means the link is not yet
    // bonded and pairing is required. Already-paired devices never see this.
    if (!promptedForPairing) {
      setStatus(
        "Waiting for pairing...\n\nIf the device LED is MAGENTA, press Button 1 " +
          "on the device to confirm pairing. This can take up to ~30s.",
      );
      promptedForPairing = true;
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
  const led = (await readRetry(chars.led)).getUint8(0);
  const nameView = await readRetry(chars.name);
  const name = decoder.decode(nameView).replace(/\0+$/, "");
  const keymapView = await readRetry(chars.keymap);
  const keymap = Array.from(new Uint8Array(keymapView.buffer));
  applyState({ led, name, keymap });
}

// ----- actions -----

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
  device = await navigator.bluetooth.requestDevice({
    acceptAllDevices: true,
    optionalServices: [UUID.ledService, UUID.configService],
  });
  device.addEventListener("gattserverdisconnected", () => {
    setConnected(false);
    setStatus("Device disconnected.");
  });

  setStatus("Connecting...");
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
  setConnected(true);
  setStatus(`Connected to ${device.name || "Clawd"}.`, "ready");
}

function disconnect() {
  if (device && device.gatt && device.gatt.connected) {
    device.gatt.disconnect();
  }
  setConnected(false);
  setStatus("Disconnected.");
}

function requireConnection() {
  if (!connected || !chars.led) {
    throw new Error("Connect to Clawd first.");
  }
}

async function setLed(level) {
  requireConnection();
  const value = Number(level);
  await writeValue(chars.led, new Uint8Array([value]));
  $("led-slider").value = String(value);
  $("led-value").textContent = String(value);
}

async function saveName() {
  requireConnection();
  const value = $("device-name").value.trim();
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
  await writeValue(chars.keymap, encodeKeymap());
}

async function sendCommand(command) {
  requireConnection();
  await writeValue(chars.command, new Uint8Array([command]));
  await sleep(150);
  await loadState();
}

// ----- wire up -----

function bindUi() {
  for (const prefix of ["b1", "b2"]) {
    populateType($(`${prefix}-type`));
    populateCodes(prefix);
    $(`${prefix}-type`).addEventListener("change", () => populateCodes(prefix));
  }

  $("connect").addEventListener("click", () => run(connect()));
  $("disconnect").addEventListener("click", () => disconnect());

  $("save-name").addEventListener("click", () =>
    run(saveName(), "Name saved. Reconnect for BLE; reboot for USB."));
  $("reset-name").addEventListener("click", () =>
    run(sendCommand(CMD_RESET_NAME), "Name reset to clawd."));

  $("led-slider").addEventListener("input", (event) => {
    $("led-value").textContent = event.target.value;
  });
  $("led-slider").addEventListener("change", (event) =>
    run(setLed(event.target.value), "LED updated."));
  for (const button of document.querySelectorAll("[data-led]")) {
    button.addEventListener("click", () => run(setLed(button.dataset.led), "LED updated."));
  }

  $("save-keys").addEventListener("click", () => run(saveKeys(), "Button mapping saved."));
  $("reset-keys").addEventListener("click", () =>
    run(sendCommand(CMD_RESET_KEYMAP), "Buttons reset to F13 / F14."));

  setConnected(false);

  if (!navigator.bluetooth) {
    setStatus(
      "Web Bluetooth is not available in this browser. Use Chrome or Edge over " +
        "https:// or http://localhost.",
      "error",
    );
  }
}

bindUi();
