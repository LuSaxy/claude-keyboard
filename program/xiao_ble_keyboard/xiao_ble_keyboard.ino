/*
 * XIAO nRF52840 - 2-Button Keyboard  (BLE + USB-HID, low power, battery)
 * ----------------------------------------------------------------------------
 * Two physical push-buttons act as a tiny keyboard. The board can type either
 * over Bluetooth LE or over the USB cable, runs from a LiPo battery, charges it,
 * and deep-sleeps to last for months.
 *
 * Board package : "Seeed nRF52 Boards"  (Tools > Board > Seeed XIAO nRF52840)
 * BLE stack     : Adafruit Bluefruit  (bluefruit.h)
 * USB stack     : Adafruit TinyUSB    (Adafruit_TinyUSB.h)
 *
 * --------------------------------------------------------------------------
 * Wiring
 * --------------------------------------------------------------------------
 *   LiPo +  --[ ON/OFF slide switch ]--> BAT+      (cuts battery when OFF)
 *   LiPo -  ----------------------------> BAT-
 *   USB-C   --> powers the board AND charges the cell automatically (BQ25101)
 *
 *   D0  --[ button 1 ]--> GND
 *   D1  --[ button 2 ]--> GND
 *   D2  --[ MODE switch ]--> GND     closed(LOW)=USB-HID enabled, open(HIGH)=BLE only
 *
 * Buttons/switch use internal pull-ups (released/open = HIGH, pressed/closed = LOW).
 *
 * --------------------------------------------------------------------------
 * Modes (MODE switch is read once at boot - flip it then RESET to change)
 * --------------------------------------------------------------------------
 *   Switch USB (D2=LOW):  cable to PC -> types over USB-HID, charges, no sleep
 *                         no cable     -> types over BLE, battery, deep sleeps
 *   Switch BLE (D2=HIGH): cable to PC -> ONLY charges (not seen as a keyboard),
 *                                        still types over BLE
 *                         no cable     -> types over BLE, battery, deep sleeps
 *
 * Deep sleep: only on battery (no USB power). Wakes (resets) on a button press;
 * the waking press is not typed. RGB LED (active LOW): short blue pulse = waiting,
 * off = connected/idle, green flash = key sent.
 * ----------------------------------------------------------------------------
 */

// ============================ Configuration =================================

#define DEVICE_NAME       "clawd"      // default BLE + USB name
#define DEVICE_NAME_MAX_LEN 15          // keep short enough to fit in advertising

#define BUTTON_1_PIN      D0           // P0.02
#define BUTTON_2_PIN      D1           // P0.03
#define MODE_SWITCH_PIN   D2           // P0.28 : LOW = USB-HID, HIGH = BLE only

// Each button sends an optional modifier + one key (a HID keycode). Examples:
//   plain 'a'        :  0,                          HID_KEY_A
//   digit '9'        :  0,                          HID_KEY_9
//   Ctrl + 9         :  KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_9
//   Ctrl + Shift + T : (KEYBOARD_MODIFIER_LEFTCTRL|KEYBOARD_MODIFIER_LEFTSHIFT), HID_KEY_T
// Modifiers: KEYBOARD_MODIFIER_LEFTCTRL / _LEFTSHIFT / _LEFTALT / _LEFTGUI (and RIGHT*).
// F13..F24 are "free" keys: defined in HID but on no real keyboard, and unbound
// by default on Windows/macOS/Linux - perfect to map in your own tools.
// For a free *combo* instead, use the Hyper modifier set:
//   (KEYBOARD_MODIFIER_LEFTCTRL|KEYBOARD_MODIFIER_LEFTSHIFT|KEYBOARD_MODIFIER_LEFTALT|KEYBOARD_MODIFIER_LEFTGUI)
#define BUTTON_1_MOD      0                            // button 1: Enter
#define BUTTON_1_CODE     HID_KEY_RETURN
#define BUTTON_2_MOD      0                            // button 2: Down Arrow
#define BUTTON_2_CODE     HID_KEY_ARROW_DOWN

#define DEBOUNCE_MS       15           // button debounce window
#define KEY_TAP_MS        8            // gap between key-down and key-up
#define LED_FLASH_MS      60           // green flash duration after a keypress

// --- User LED (controlled over BLE) ---
#define LED_USER_PIN      D3           // P0.29 : external LED(s), active HIGH, PWM dimmable
#define LED_USER_DEFAULT  0            // brightness at boot (0 = off .. 255 = full)

// --- Sleep ---
#define IDLE_SLEEP_S      600          // connected & idle this long -> deep sleep (0 = never)
#define ADV_TIMEOUT_S     180          // advertising with no connection -> deep sleep
#define POWERDOWN_FLASH   1            // 1 = deep power-down QSPI flash before sleep
                                       //     (needs "Adafruit SPIFlash" + "SdFat - Adafruit Fork")

// --- Battery ---
#define ENABLE_BATTERY    1            // 1 = report battery % (BLE) + low-voltage cutoff
#define VBAT_CUTOFF_MV    3200         // below this (on battery) -> deep sleep to protect cell
#define BATTERY_PERIOD_MS 30000        // how often to measure
#define CHARGE_100MA      0            // 0 = 50 mA charge (small cells), 1 = 100 mA
#define VBAT_MV_PER_LSB   0.87890625f  // 3.6 V ref / 4096 (12-bit)
#define VBAT_DIVIDER      2.9608f      // (1.0M + 0.51M) / 0.51M  --  CALIBRATE per board

// --- Security / pairing ---
#define REQUIRE_PAIR_CONFIRM    1      // 1 = a new pairing must be confirmed on the device
#define PAIR_CONFIRM_TIMEOUT_MS 30000  // window to accept/reject a new pairing
#define BOND_RESET_HOLD_MS      5000   // hold BOTH buttons at boot this long to wipe bonds
                                       // Button 1 = accept, Button 2 = reject a pairing.

// ============================================================================

#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>          // also fixes Serial compile on Seeed nRF52
#if POWERDOWN_FLASH
#include <Adafruit_SPIFlash.h>
#endif
#include <Adafruit_LittleFS.h>        // tiny filesystem on internal flash (saved settings)
#include <InternalFileSystem.h>       // provides the global "InternalFS" instance

// Battery / charge pins are provided by the Seeed XIAO nRF52840 variant:
//   PIN_VBAT (32)             - P0.31 battery-voltage ADC tap
//   VBAT_ENABLE (14)          - P0.14 read-enable (drive LOW to enable divider)
//   PIN_CHARGING_CURRENT (22) - P0.13 (HIGH = 50 mA, LOW = 100 mA)

// ----------------------------- Services / devices ---------------------------

BLEDis          bledis;
BLEHidAdafruit  blehid;
#if ENABLE_BATTERY
BLEBas          blebas;
#endif

// Custom "user LED" service: a single writable/readable byte = LED brightness
// (0 = off .. 255 = full). UUIDs are random 128-bit values; write/read require
// an encrypted link so only bonded hosts can drive the LED.
//   Service        : 494dceab-1533-4012-afaa-20d5acfc6ecf
//   Characteristic : bc6df096-9c5a-4034-89d2-8440c14d2ab0
// (Bluefruit stores 128-bit UUIDs LSB-first, so the bytes are reversed below.)
uint8_t const LED_SVC_UUID[16] = {
  0xcf,0x6e,0xfc,0xac,0xd5,0x20,0xaa,0xaf,
  0x12,0x40,0x33,0x15,0xab,0xce,0x4d,0x49
};
uint8_t const LED_CHR_UUID[16] = {
  0xb0,0x2a,0x4d,0xc1,0x40,0x84,0xd2,0x89,
  0x34,0x40,0x5a,0x9c,0x96,0xf0,0x6d,0xbc
};
BLEService        ledSvc(LED_SVC_UUID);
BLECharacteristic ledChr(LED_CHR_UUID);
uint8_t           ledLevel = LED_USER_DEFAULT;

// Custom "config" service: change what each button sends (keyboard or media key)
// and the advertised device name at runtime, then persist it. Encrypted
// permission => only bonded hosts can write.
//   Service                   : e0934030-b408-4dcc-b4ca-56d83d7c5fbc
//   Keymap char       (8 B RW): e0c5d214-8809-4243-afa2-9862d3628c38
//   Device name char (<=15 RW): 8adf91b2-c9a1-4c1a-8b76-2ff5be6b86de
//   Command char      (1 B  W): 37508895-a542-4ee0-9ab4-b109ce4717de
uint8_t const CFG_SVC_UUID[16] = {
  0xbc,0x5f,0x7c,0x3d,0xd8,0x56,0xca,0xb4,
  0xcc,0x4d,0x08,0xb4,0x30,0x40,0x93,0xe0
};
uint8_t const KEYMAP_CHR_UUID[16] = {
  0x38,0x8c,0x62,0xd3,0x62,0x98,0xa2,0xaf,
  0x43,0x42,0x09,0x88,0x14,0xd2,0xc5,0xe0
};
uint8_t const NAME_CHR_UUID[16] = {
  0xde,0x86,0x6b,0xbe,0xf5,0x2f,0x76,0x8b,
  0x1a,0x4c,0xa1,0xc9,0xb2,0x91,0xdf,0x8a
};
uint8_t const CMD_CHR_UUID[16] = {
  0xde,0x17,0x47,0xce,0x09,0xb1,0xb4,0x9a,
  0xe0,0x4e,0x42,0xa5,0x95,0x88,0x50,0x37
};
BLEService        cfgSvc(CFG_SVC_UUID);
BLECharacteristic keymapChr(KEYMAP_CHR_UUID);
BLECharacteristic nameChr(NAME_CHR_UUID);
BLECharacteristic cmdChr(CMD_CHR_UUID);

uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_KEYBOARD() };
Adafruit_USBD_HID usb_hid;

#if POWERDOWN_FLASH
Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash            flash(&flashTransport);
#endif

// ----------------------------- State ----------------------------------------

struct Button {
  uint8_t  pin;
  bool     stable;
  bool     lastRead;
  uint32_t lastEdge;
};

Button btn1 = { BUTTON_1_PIN, HIGH, HIGH, 0 };
Button btn2 = { BUTTON_2_PIN, HIGH, HIGH, 0 };

bool     usbMode      = false;   // true = MODE switch selected USB-HID
uint32_t ledFlashUntil = 0;
uint32_t lastActivity  = 0;
uint32_t advStart      = 0;
uint32_t lastBat       = 0;

// --- Configurable key mapping (set over BLE, saved to internal flash) ---
#define KEYMAP_TYPE_KEYBOARD  0      // mod + code is a keyboard key/combo
#define KEYMAP_TYPE_CONSUMER  1      // code is a media/consumer usage (BLE only)
#define KEYMAP_FILE           "/keymap.bin"
#define KEYMAP_VERSION        1
#define NAME_FILE             "/name.bin"
#define NAME_VERSION          1
#define CMD_RESET_DEFAULTS    0x01   // restore compiled keymap defaults
#define CMD_RESET_NAME        0x02   // restore compiled device name default

struct KeyMap {
  uint8_t  type;   // KEYMAP_TYPE_KEYBOARD or KEYMAP_TYPE_CONSUMER
  uint8_t  mod;    // keyboard modifier bitmask (ignored for consumer)
  uint16_t code;   // keyboard: HID_KEY_* ; consumer: HID_USAGE_CONSUMER_*
};

KeyMap key1 = { KEYMAP_TYPE_KEYBOARD, BUTTON_1_MOD, BUTTON_1_CODE };
KeyMap key2 = { KEYMAP_TYPE_KEYBOARD, BUTTON_2_MOD, BUTTON_2_CODE };
bool   keymapDirty = false;          // set in BLE callback, flushed to flash in loop()

char deviceName[DEVICE_NAME_MAX_LEN + 1] = DEVICE_NAME;
char usbHidName[DEVICE_NAME_MAX_LEN + 10];
bool nameDirty = false;              // set in BLE callback, flushed to flash in loop()
bool advRefreshPending = false;      // refresh advertising payload after a rename

// ----------------------------- Helpers --------------------------------------

void setRGB(bool r, bool g, bool b) {
  digitalWrite(LED_RED,   r ? LOW : HIGH);
  digitalWrite(LED_GREEN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE,  b ? LOW : HIGH);
}

// Apply a brightness (0 = off) to the BLE-controlled user LED on LED_USER_PIN.
// Always go through analogWrite(): once a pin is owned by the HwPWM peripheral a
// later digitalWrite() is ignored (PWM keeps driving the pin), so writing 0 with
// digitalWrite would leave the LED stuck at the last brightness. analogWrite(.,0)
// is 0% duty = pin held LOW = LED off.
void applyUserLed(uint8_t level) {
  ledLevel = level;
  analogWrite(LED_USER_PIN, level);   // 0 = 0% duty (off) .. 255 = full (active HIGH)
}

// A bonded host wrote the LED characteristic: byte 0 = brightness (0..255).
// Runs in Bluefruit's callback task; touching GPIO/PWM here is fine.
void led_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl;
  (void) chr;
  if (len < 1) return;
  applyUserLed(data[0]);
  ledChr.write8(ledLevel);                            // keep the readable value in sync
}

// Register the custom LED service. Service must begin() before its characteristic,
// and both before advertising/connection. Encrypted permission => bonded-only.
void setupLedService() {
  ledSvc.begin();
  ledChr.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  ledChr.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  ledChr.setFixedLen(1);
  ledChr.setWriteCallback(led_write_cb);
  ledChr.begin();
  ledChr.write8(ledLevel);
}

// ---- Configurable key mapping: pack/unpack, persistence, BLE service ----

// 8-byte wire format: [t1,m1,code1_lo,code1_hi, t2,m2,code2_lo,code2_hi].
void keymapToBytes(uint8_t out[8]) {
  out[0] = key1.type; out[1] = key1.mod;
  out[2] = (uint8_t)(key1.code & 0xFF); out[3] = (uint8_t)(key1.code >> 8);
  out[4] = key2.type; out[5] = key2.mod;
  out[6] = (uint8_t)(key2.code & 0xFF); out[7] = (uint8_t)(key2.code >> 8);
}
void bytesToKeymap(const uint8_t in[8]) {
  key1.type = in[0]; key1.mod = in[1];
  key1.code = (uint16_t)in[2] | ((uint16_t)in[3] << 8);
  key2.type = in[4]; key2.mod = in[5];
  key2.code = (uint16_t)in[6] | ((uint16_t)in[7] << 8);
}
void resetKeymapDefaults() {
  key1.type = KEYMAP_TYPE_KEYBOARD; key1.mod = BUTTON_1_MOD; key1.code = BUTTON_1_CODE;
  key2.type = KEYMAP_TYPE_KEYBOARD; key2.mod = BUTTON_2_MOD; key2.code = BUTTON_2_CODE;
}

// Persist current mapping to internal flash as [VERSION][8 bytes].
void saveKeymap() {
  uint8_t buf[9];
  buf[0] = KEYMAP_VERSION;
  keymapToBytes(buf + 1);
  InternalFS.remove(KEYMAP_FILE);                       // truncate / overwrite
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (f.open(KEYMAP_FILE, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    f.write(buf, sizeof(buf));
    f.close();
  }
}

// Load a saved mapping (if present and version matches), else keep defaults.
void loadKeymap() {
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (!f.open(KEYMAP_FILE, Adafruit_LittleFS_Namespace::FILE_O_READ)) return;
  uint8_t buf[9];
  int n = f.read(buf, sizeof(buf));
  f.close();
  if (n == (int)sizeof(buf) && buf[0] == KEYMAP_VERSION) bytesToKeymap(buf + 1);
}

void setDefaultDeviceName() {
  strncpy(deviceName, DEVICE_NAME, DEVICE_NAME_MAX_LEN);
  deviceName[DEVICE_NAME_MAX_LEN] = '\0';
}

// Decode a user-provided name into printable ASCII. Empty names are rejected;
// leading/trailing spaces are trimmed so the device remains easy to discover.
bool cleanDeviceName(char out[DEVICE_NAME_MAX_LEN + 1], const uint8_t* data, uint16_t len) {
  uint8_t n = 0;
  for (uint16_t i = 0; i < len && n < DEVICE_NAME_MAX_LEN; i++) {
    char c = (char)data[i];
    if (c == '\0') break;
    if (c < 32 || c > 126) continue;
    if (n == 0 && c == ' ') continue;
    out[n++] = c;
  }
  while (n > 0 && out[n - 1] == ' ') n--;
  out[n] = '\0';
  return n > 0;
}

bool setDeviceNameFromBytes(const uint8_t* data, uint16_t len) {
  char next[DEVICE_NAME_MAX_LEN + 1];
  if (!cleanDeviceName(next, data, len)) return false;
  if (strcmp(deviceName, next) == 0) return false;
  strncpy(deviceName, next, sizeof(deviceName));
  deviceName[DEVICE_NAME_MAX_LEN] = '\0';
  return true;
}

void saveDeviceName() {
  uint8_t len = (uint8_t)strlen(deviceName);
  uint8_t buf[2 + DEVICE_NAME_MAX_LEN];
  buf[0] = NAME_VERSION;
  buf[1] = len;
  memcpy(buf + 2, deviceName, len);
  InternalFS.remove(NAME_FILE);
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (f.open(NAME_FILE, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    f.write(buf, 2 + len);
    f.close();
  }
}

void loadDeviceName() {
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (!f.open(NAME_FILE, Adafruit_LittleFS_Namespace::FILE_O_READ)) return;
  uint8_t buf[2 + DEVICE_NAME_MAX_LEN];
  int n = f.read(buf, sizeof(buf));
  f.close();
  if (n < 2 || buf[0] != NAME_VERSION || buf[1] > DEVICE_NAME_MAX_LEN) return;
  if (n < (int)(2 + buf[1])) return;
  setDeviceNameFromBytes(buf + 2, buf[1]);
}

void writeDeviceNameCharacteristic() {
  nameChr.write(deviceName);
}

// Bonded host wrote the keymap characteristic (8 bytes): apply + flag for save.
void keymap_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl; (void) chr;
  if (len < 8) return;
  bytesToKeymap(data);
  uint8_t b[8]; keymapToBytes(b);
  keymapChr.write(b, sizeof(b));                        // keep readable value in sync
  keymapDirty = true;                                   // flash write deferred to loop()
}

// Bonded host wrote the device-name characteristic: apply + flag for save.
// BLE advertising shows the new name after disconnect/restart; USB uses it after reboot.
void name_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl; (void) chr;
  if (setDeviceNameFromBytes(data, len)) {
    Bluefruit.setName(deviceName);
    nameDirty = true;
    advRefreshPending = true;
  }
  writeDeviceNameCharacteristic();                       // keep readable value in sync
}

// Command characteristic: 0x01 = reset keys, 0x02 = reset device name.
void cmd_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl; (void) chr;
  if (len < 1) return;
  if (data[0] == CMD_RESET_DEFAULTS) {
    resetKeymapDefaults();
    uint8_t b[8]; keymapToBytes(b);
    keymapChr.write(b, sizeof(b));
    keymapDirty = true;
  } else if (data[0] == CMD_RESET_NAME) {
    setDefaultDeviceName();
    Bluefruit.setName(deviceName);
    writeDeviceNameCharacteristic();
    nameDirty = true;
    advRefreshPending = true;
  }
}

void setupConfigService() {
  cfgSvc.begin();

  keymapChr.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  keymapChr.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  keymapChr.setFixedLen(8);
  keymapChr.setWriteCallback(keymap_write_cb);
  keymapChr.begin();
  uint8_t b[8]; keymapToBytes(b);
  keymapChr.write(b, sizeof(b));

  nameChr.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  nameChr.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  nameChr.setMaxLen(DEVICE_NAME_MAX_LEN);
  nameChr.setWriteCallback(name_write_cb);
  nameChr.begin();
  writeDeviceNameCharacteristic();

  cmdChr.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  cmdChr.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  cmdChr.setFixedLen(1);
  cmdChr.setWriteCallback(cmd_write_cb);
  cmdChr.begin();
}

// Is the board powered from USB right now? (VBUS present)
static inline bool usbPowered() {
  return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}

// Send "modifier + keycode" as a tap over USB-HID if available, else over BLE.
void sendKey(uint8_t modifier, uint8_t keycode) {
  uint8_t keys[6] = { keycode, 0, 0, 0, 0, 0 };
  if (usbMode && usb_hid.ready()) {
    usb_hid.keyboardReport(0, modifier, keys);
    delay(KEY_TAP_MS);
    usb_hid.keyboardRelease(0);
    return;
  }
  if (Bluefruit.connected()) {
    blehid.keyboardReport(modifier, keys);
    delay(KEY_TAP_MS);
    blehid.keyRelease();
  }
}

// Send a media/consumer key. BLE only: the USB-HID descriptor here is keyboard-only,
// so media keys require a connected BLE host. Codes: HID_USAGE_CONSUMER_*.
void sendConsumer(uint16_t usage) {
  if (Bluefruit.connected()) {
    blehid.consumerKeyPress(usage);
    delay(KEY_TAP_MS);
    blehid.consumerKeyRelease();
  }
}

// Dispatch a configured button mapping to the correct HID path.
void sendMapping(const KeyMap& k) {
  if (k.type == KEYMAP_TYPE_CONSUMER) sendConsumer(k.code);
  else                                sendKey(k.mod, (uint8_t)k.code);
}

bool pollPressed(Button& b) {
  bool reading = digitalRead(b.pin);
  if (reading != b.lastRead) {
    b.lastRead = reading;
    b.lastEdge = millis();
  }
  if ((millis() - b.lastEdge) > DEBOUNCE_MS && reading != b.stable) {
    b.stable = reading;
    if (b.stable == LOW) return true;
  }
  return false;
}

void updateLed(bool active) {
  uint32_t now = millis();
  if (now < ledFlashUntil)      setRGB(false, true, false);          // green key flash
  else if (active)              setRGB(false, false, false);         // connected & idle
  else                          setRGB(false, false, (now % 2000) < 15); // blue pulse
}

#if ENABLE_BATTERY
uint16_t readVbatMv() {
  uint32_t raw = analogRead(PIN_VBAT);                 // divider enabled since boot
  return (uint16_t)((float)raw * VBAT_MV_PER_LSB * VBAT_DIVIDER);
}

uint8_t vbatPercent(uint16_t mv) {
  if (mv >= 4200) return 100;
  if (mv <= 3300) return 0;
  return (uint8_t)(((uint32_t)(mv - 3300) * 100) / (4200 - 3300));
}
#endif

// Lowest-power state. Only reached on battery. Wake = button press = reset.
void enterDeepSleep() {
  setRGB(false, false, false);

  for (uint16_t h = 0; h < BLE_MAX_CONNECTION; h++) {
    BLEConnection* conn = Bluefruit.Connection(h);
    if (conn && conn->connected()) conn->disconnect();
  }
  Bluefruit.Advertising.stop();
  delay(50);

#if POWERDOWN_FLASH
  flash.begin();
  flashTransport.runCommand(0xB9);       // QSPI flash deep power-down (~1 µA)
  delay(5);
  flash.end();
#endif

  pinMode(BUTTON_1_PIN, INPUT_PULLUP_SENSE);   // arm buttons as wake sources
  pinMode(BUTTON_2_PIN, INPUT_PULLUP_SENSE);

  sd_power_system_off();                  // never returns
  while (true) { }
}

void startAdv() {
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
  advStart = millis();
}

// ----------------------------- BLE callbacks --------------------------------

void connect_callback(uint16_t conn_handle) {
  (void) conn_handle;
  lastActivity = millis();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;
  (void) reason;
  advStart = millis();
}

// Ask for an on-device confirmation before allowing a new pairing/bond.
// Runs in Bluefruit's callback task, so blocking here is fine.
//   Button 1 (D0) = accept    Button 2 (D1) = reject    timeout = reject
bool pair_confirm_callback(uint16_t conn_handle, uint8_t const passkey[6], bool match_request) {
  (void) conn_handle;
  (void) passkey;
  (void) match_request;
#if REQUIRE_PAIR_CONFIRM
  uint32_t t = millis();
  bool accepted = false;
  while ((millis() - t) < PAIR_CONFIRM_TIMEOUT_MS) {
    setRGB(true, false, true);                       // magenta: "confirm pairing?"
    if (digitalRead(BUTTON_1_PIN) == LOW) { accepted = true;  break; }   // accept
    if (digitalRead(BUTTON_2_PIN) == LOW) { accepted = false; break; }   // reject
    delay(20);
  }
  setRGB(false, false, false);
  return accepted;                                   // timeout -> reject
#else
  return true;
#endif
}

// LED feedback once a pairing finishes (green = ok, red = failed/rejected).
void pair_complete_callback(uint16_t conn_handle, uint8_t auth_status) {
  (void) conn_handle;
  bool ok = (auth_status == BLE_GAP_SEC_STATUS_SUCCESS);
  setRGB(!ok, ok, false);                            // ok -> green, fail -> red
  delay(350);
  setRGB(false, false, false);
}

// Hold BOTH buttons at boot for BOND_RESET_HOLD_MS to wipe all stored bonds.
void maybeResetBonds() {
  if (digitalRead(BUTTON_1_PIN) != LOW || digitalRead(BUTTON_2_PIN) != LOW) return;
  uint32_t t = millis();
  while (digitalRead(BUTTON_1_PIN) == LOW && digitalRead(BUTTON_2_PIN) == LOW) {
    setRGB(true, false, false);                      // red while holding
    if ((millis() - t) >= BOND_RESET_HOLD_MS) {
      Bluefruit.Periph.clearBonds();
      for (int i = 0; i < 3; i++) {                  // 3 red blinks = bonds cleared
        setRGB(true, false, false); delay(120);
        setRGB(false, false, false); delay(120);
      }
      return;
    }
    delay(20);
  }
  setRGB(false, false, false);
}

// ----------------------------- Arduino entry points -------------------------

void setup() {
  // Read the MODE switch first, then register USB-HID as early as possible so
  // it is part of USB enumeration (the descriptor set is fixed at boot).
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  delay(1);
  usbMode = (digitalRead(MODE_SWITCH_PIN) == LOW);   // LOW (to GND) = USB-HID

  InternalFS.begin();
  loadDeviceName();

  if (usbMode) {
    snprintf(usbHidName, sizeof(usbHidName), "%s Keyboard", deviceName);
    TinyUSBDevice.setProductDescriptor(deviceName);
    usb_hid.setStringDescriptor(usbHidName);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setBootProtocol(HID_ITF_PROTOCOL_KEYBOARD);
    usb_hid.setPollInterval(2);
    usb_hid.begin();
  }

  Serial.begin(115200);

  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE,  OUTPUT);
  setRGB(false, false, false);

  pinMode(LED_USER_PIN, OUTPUT);          // BLE-controlled external LED
  applyUserLed(LED_USER_DEFAULT);

  // Charge current select (50 mA default, 100 mA if CHARGE_100MA).
  pinMode(PIN_CHARGING_CURRENT, OUTPUT);
  digitalWrite(PIN_CHARGING_CURRENT, CHARGE_100MA ? LOW : HIGH);

#if ENABLE_BATTERY
  // Keep the battery-sense divider enabled (P0.14 LOW) and configure the ADC.
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  analogReference(AR_DEFAULT);          // 3.6 V full scale
  analogReadResolution(12);
#endif

  // Load any saved key mapping from internal flash (keeps compiled defaults if none).
  loadKeymap();

  // BLE is always available (primary, or fallback when USB is unplugged).
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName(deviceName);
  Bluefruit.autoConnLed(false);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Require an on-device confirmation for any NEW pairing (Button 1 = accept,
  // Button 2 = reject). Already-bonded hosts reconnect without confirmation.
  Bluefruit.Security.setIOCaps(true, true, false);            // DisplayYesNo -> numeric compare
  Bluefruit.Security.setPairPasskeyCallback(pair_confirm_callback);  // also enables MITM
  Bluefruit.Security.setPairCompleteCallback(pair_complete_callback);

  // Boot-time gesture: hold BOTH buttons ~5 s to wipe all stored bonds.
  maybeResetBonds();

  bledis.setManufacturer("Seeed Studio");
  bledis.setModel("XIAO nRF52840");
  bledis.begin();

  blehid.begin();
#if ENABLE_BATTERY
  blebas.begin();
  blebas.write(100);
#endif

  setupLedService();                      // custom BLE-controlled LED service
  setupConfigService();                   // runtime-configurable, persisted key mapping

  lastActivity = millis();
  lastBat      = millis();
  startAdv();
}

void loop() {
  uint32_t now = millis();

  bool usbReady = usbMode && usb_hid.ready();
  bool bleReady = Bluefruit.connected();
  bool active   = usbReady || bleReady;
  bool onUsbPwr = usbPowered();

  bool p1 = pollPressed(btn1);
  bool p2 = pollPressed(btn2);

  if (active) {
    if (p1) { sendMapping(key1); ledFlashUntil = now + LED_FLASH_MS; lastActivity = now; }
    if (p2) { sendMapping(key2); ledFlashUntil = now + LED_FLASH_MS; lastActivity = now; }
  }

  // Persist changed settings outside BLE callbacks (flash writes are blocking).
  if (keymapDirty) { keymapDirty = false; saveKeymap(); }
  if (nameDirty) { nameDirty = false; saveDeviceName(); }
  if (advRefreshPending && !Bluefruit.connected()) { advRefreshPending = false; startAdv(); }

#if ENABLE_BATTERY
  if (now - lastBat > BATTERY_PERIOD_MS) {
    lastBat = now;
    uint16_t mv = readVbatMv();
    blebas.write(vbatPercent(mv));
    // Protect the cell: cut off only on battery, ignore implausibly low reads.
    if (!onUsbPwr && mv > 2000 && mv < VBAT_CUTOFF_MV) enterDeepSleep();
  }
#endif

  // Deep sleep only when running on the battery (never while USB-powered).
  if (!onUsbPwr) {
    if (bleReady) {
#if IDLE_SLEEP_S > 0
      if ((now - lastActivity) > (uint32_t)IDLE_SLEEP_S * 1000UL) enterDeepSleep();
#endif
    } else {
      if ((now - advStart) > (uint32_t)ADV_TIMEOUT_S * 1000UL) enterDeepSleep();
    }
  }

  updateLed(active);
  delay(5);
}
