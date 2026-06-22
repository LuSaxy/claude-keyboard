// All Bluetooth work runs natively in Rust (see src-tauri/src/ble.rs).
// This file only drives the UI and calls the backend through `invoke`.
const invoke = window.__TAURI__.core.invoke;

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
let connected = false;

function setStatus(message, kind = "") {
  const status = $("status");
  status.textContent = message;
  status.classList.toggle("ready", kind === "ready");
  status.classList.toggle("error", kind === "error");
}

function setConnected(state) {
  connected = state;
  $("disconnect").disabled = !state;
  for (const card of document.querySelectorAll("[data-requires-connection]")) {
    card.setAttribute("aria-disabled", String(!state));
  }
}

async function call(command, args) {
  return invoke(command, args);
}

function run(promise, okMessage) {
  promise
    .then((result) => {
      if (okMessage) setStatus(okMessage, "ready");
      return result;
    })
    .catch((error) => setStatus(String(error), "error"));
}

// ----- Buttons / keymap -----

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
  return [
    b1.type, b1.mod, b1.code & 0xff, b1.code >> 8,
    b2.type, b2.mod, b2.code & 0xff, b2.code >> 8,
  ];
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

// ----- Connection flow -----

async function scan() {
  setStatus("Scanning...");
  $("connect").disabled = true;
  const devices = await call("scan_devices", { seconds: 4 });
  const select = $("devices");
  select.replaceChildren(
    ...devices.map((d) => {
      const label = d.name ? d.name : "(no name)";
      const tag = d.looks_like_clawd ? "  ★" : "";
      return new Option(`${label}${tag}  —  ${d.id}`, d.id);
    }),
  );
  if (devices.length > 0) {
    select.selectedIndex = 0;
    $("connect").disabled = false;
    setStatus(`Found ${devices.length} device(s). Select one and connect.`, "ready");
  } else {
    setStatus("No devices found. Make sure Clawd is on and paired.", "error");
  }
}

async function connect() {
  const id = $("devices").value;
  if (!id) {
    setStatus("Select a device first.", "error");
    return;
  }
  setStatus("Connecting...");
  const state = await call("connect_device", { id });
  applyState(state);
  setConnected(true);
  setStatus(`Connected to ${state.name || "Clawd"}`, "ready");
}

async function disconnect() {
  await call("disconnect");
  setConnected(false);
  setStatus("Disconnected");
}

// ----- Wire up -----

function bindUi() {
  for (const prefix of ["b1", "b2"]) {
    populateType($(`${prefix}-type`));
    populateCodes(prefix);
    $(`${prefix}-type`).addEventListener("change", () => populateCodes(prefix));
  }

  $("scan").addEventListener("click", () => run(scan()));
  $("connect").addEventListener("click", () => run(connect()));
  $("disconnect").addEventListener("click", () => run(disconnect()));

  $("save-name").addEventListener("click", () =>
    run(call("set_device_name", { name: $("device-name").value.trim() }),
      "Name saved. Reconnect for BLE; reboot for USB."));
  $("reset-name").addEventListener("click", () =>
    run(call("reset_name").then(applyState), "Name reset to clawd"));

  $("led-slider").addEventListener("input", (event) => {
    $("led-value").textContent = event.target.value;
  });
  $("led-slider").addEventListener("change", (event) =>
    run(call("set_led", { level: Number(event.target.value) }), "LED updated"));
  for (const button of document.querySelectorAll("[data-led]")) {
    button.addEventListener("click", () => {
      const level = Number(button.dataset.led);
      $("led-slider").value = String(level);
      $("led-value").textContent = String(level);
      run(call("set_led", { level }), "LED updated");
    });
  }

  $("save-keys").addEventListener("click", () =>
    run(call("set_keymap", { bytes: encodeKeymap() }), "Button mapping saved"));
  $("reset-keys").addEventListener("click", () =>
    run(call("reset_keymap").then(applyState), "Buttons reset to F13 / F14"));

  setConnected(false);
}

bindUi();
