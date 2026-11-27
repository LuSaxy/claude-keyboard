/*
 * XIAO nRF52840 - Clawd Keyboard  (BLE + USB-HID, low power, battery)
 * ----------------------------------------------------------------------------
 * Two key switches and two side shoulder buttons (L1/R1) act as a tiny keyboard. The board can type either
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
 *   D4  --[ L1 shoulder ]--> GND    (V2 board: SW3, side push)
 *   D5  --[ R1 shoulder ]--> GND    (V2 board: SW4, side push)
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
 * Deep sleep: only on battery (no USB power). Wakes (resets) on any button press;
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
#define BUTTON_L1_PIN     D4           // P0.04 : left shoulder (SW3)
#define BUTTON_R1_PIN     D5           // P0.05 : right shoulder (SW4)

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
#define BUTTON_1_MOD      0                            // button 1 (left key): Escape
#define BUTTON_1_CODE     HID_KEY_ESCAPE
#define BUTTON_2_MOD      0                            // button 2 (right key): Enter
#define BUTTON_2_CODE     HID_KEY_RETURN
#define BUTTON_L1_MOD     KEYBOARD_MODIFIER_LEFTSHIFT  // L1 shoulder: Shift+Tab
#define BUTTON_L1_CODE    HID_KEY_TAB
#define BUTTON_R1_MOD     0                            // R1 shoulder: Tab
#define BUTTON_R1_CODE    HID_KEY_TAB

// Motion gestures (XIAO nRF52840 Sense only, see motion.h). They use the same
// keymap as the buttons and can be switched off from the control app.
#define TAP_MOD           0                            // double tap on the case: Escape
#define TAP_CODE          HID_KEY_ESCAPE
#define TILT_LEFT_MOD     0                            // tilt left: Arrow Left
#define TILT_LEFT_CODE    HID_KEY_ARROW_LEFT
#define TILT_RIGHT_MOD    0                            // tilt right: Arrow Right
#define TILT_RIGHT_CODE   HID_KEY_ARROW_RIGHT
#define TILT_FWD_MOD      0                            // tilt forward (top away): Arrow Up
#define TILT_FWD_CODE     HID_KEY_ARROW_UP
#define TILT_BACK_MOD     0                            // tilt back (top towards you): Arrow Down
#define TILT_BACK_CODE    HID_KEY_ARROW_DOWN

#define DEBOUNCE_MS       15           // button debounce window
#define SHOULDER_CHORD_MS 120          // L1 + R1 within this = hold-to-tilt (Sense, tilt on)
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
#define PIN_CHG_STATUS    23           // P0.17, ~CHG of the BQ25101 (LOW = charging)

// --- Security / pairing ---
#define REQUIRE_PAIR_CONFIRM    1      // 1 = a new pairing must be confirmed on the device
#define PAIR_CONFIRM_TIMEOUT_MS 30000  // window to accept/reject a new pairing
#define BOND_RESET_HOLD_MS      5000   // hold BOTH buttons at boot this long to wipe bonds
#define SETTINGS_RESET_HOLD_MS  5000   // hold L1 + R1 at boot this long to wipe saved settings
#define BLE_LINKS               2      // a computer typing + the control app at the same time
                                       // Button 1 = accept, Button 2 = reject a pairing.

// ============================================================================

#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>          // also fixes Serial compile on Seeed nRF52
#if POWERDOWN_FLASH
#include <Adafruit_SPIFlash.h>
#endif
#include <Adafruit_LittleFS.h>        // tiny filesystem on internal flash (saved settings)
#include <InternalFileSystem.h>       // provides the global "InternalFS" instance
#include "motion.h"                   // Sense motion sensor (no-op on other boards)

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
//   Keymap char  (16 B RW, 8 B accepted): e0c5d214-8809-4243-afa2-9862d3628c38
//   Device name char (<=15 RW): 8adf91b2-c9a1-4c1a-8b76-2ff5be6b86de
//   Command char      (1 B  W): 37508895-a542-4ee0-9ab4-b109ce4717de
//   Motion char       (2 B R/W): 8f1e6c2a-4b7d-4e2b-9a61-3c5d2e8b7f10
//                     [0] = enabled gestures (bit0 wake, bit1 tap,
//                           bit2 tilt left/right, bit3 tilt forward/back)
//                     [1] = 1 if this board has the motion sensor (read only)
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
uint8_t const MOTION_CHR_UUID[16] = {
  0x10,0x7f,0x8b,0x2e,0x5d,0x3c,0x61,0x9a,
  0x2b,0x4e,0x7d,0x4b,0x2a,0x6c,0x1e,0x8f
};
BLECharacteristic motionChr(MOTION_CHR_UUID);

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
Button btnL1 = { BUTTON_L1_PIN, HIGH, HIGH, 0 };
Button btnR1 = { BUTTON_R1_PIN, HIGH, HIGH, 0 };

bool     usbMode      = false;   // true = MODE switch selected USB-HID
uint32_t ledFlashUntil = 0;
uint32_t lastActivity  = 0;
uint32_t advStart      = 0;
uint32_t lastBat       = 0;

// --- Configurable key mapping (set over BLE, saved to internal flash) ---
#define KEYMAP_TYPE_KEYBOARD  0      // mod + code is a keyboard key/combo
#define KEYMAP_TYPE_CONSUMER  1      // code is a media/consumer usage (BLE only)
#define KEYMAP_FILE           "/keymap.bin"
#define KEYMAP_VERSION        4      // 4 = 4 buttons + 5 gestures (36 bytes); 3 = 28; 2 = 16; 1 = 8
#define KEYMAP_KEYS           9
#define KEYMAP_LEN            (KEYMAP_KEYS * 4)
#define NAME_FILE             "/name.bin"
#define NAME_VERSION          1
#define CMD_RESET_DEFAULTS    0x01   // restore compiled keymap defaults
#define CMD_RESET_NAME        0x02   // restore compiled device name default
#define CMD_RESET_ALL         0x03   // wipe all saved settings and reboot (bonds kept)

struct KeyMap {
  uint8_t  type;   // KEYMAP_TYPE_KEYBOARD or KEYMAP_TYPE_CONSUMER
  uint8_t  mod;    // keyboard modifier bitmask (ignored for consumer)
  uint16_t code;   // keyboard: HID_KEY_* ; consumer: HID_USAGE_CONSUMER_*
};

// Order on the wire and in flash: button 1, button 2, L1, R1, double tap,
// tilt left, tilt right, tilt forward, tilt back.
#define KEY_TAP         4
#define KEY_TILT_LEFT   5
#define KEY_TILT_RIGHT  6
#define KEY_TILT_FWD    7
#define KEY_TILT_BACK   8
const KeyMap defaultKeys[KEYMAP_KEYS] = {
  { KEYMAP_TYPE_KEYBOARD, BUTTON_1_MOD,    BUTTON_1_CODE    },
  { KEYMAP_TYPE_KEYBOARD, BUTTON_2_MOD,    BUTTON_2_CODE    },
  { KEYMAP_TYPE_KEYBOARD, BUTTON_L1_MOD,   BUTTON_L1_CODE   },
  { KEYMAP_TYPE_KEYBOARD, BUTTON_R1_MOD,   BUTTON_R1_CODE   },
  { KEYMAP_TYPE_KEYBOARD, TAP_MOD,         TAP_CODE         },
  { KEYMAP_TYPE_KEYBOARD, TILT_LEFT_MOD,   TILT_LEFT_CODE   },
  { KEYMAP_TYPE_KEYBOARD, TILT_RIGHT_MOD,  TILT_RIGHT_CODE  },
  { KEYMAP_TYPE_KEYBOARD, TILT_FWD_MOD,    TILT_FWD_CODE    },
  { KEYMAP_TYPE_KEYBOARD, TILT_BACK_MOD,   TILT_BACK_CODE   },
};
KeyMap keys[KEYMAP_KEYS];
bool   keymapDirty = false;          // set in BLE callback, flushed to flash in loop()
bool   motionDirty = false;          // gesture switches changed, flushed in loop()
bool   resetAllPending = false;      // CMD_RESET_ALL received, handled in loop()
#define MOTION_FILE           "/motion.bin"

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

// Wire format, 4 bytes per entry: [type, mod, code_lo, code_hi], in the order
// button 1, button 2, L1, R1, tap, tilt left, tilt right, tilt forward, tilt
// back (36 bytes). Shorter writes from older apps (8, 16 or 28 bytes) set only
// the first entries.
void keymapToBytes(uint8_t out[KEYMAP_LEN]) {
  for (uint8_t i = 0; i < KEYMAP_KEYS; i++) {
    out[i * 4 + 0] = keys[i].type;
    out[i * 4 + 1] = keys[i].mod;
    out[i * 4 + 2] = (uint8_t)(keys[i].code & 0xFF);
    out[i * 4 + 3] = (uint8_t)(keys[i].code >> 8);
  }
}
void bytesToKeymap(const uint8_t* in, uint16_t len) {
  for (uint8_t i = 0; i < KEYMAP_KEYS && (uint16_t)(i * 4 + 4) <= len; i++) {
    keys[i].type = in[i * 4 + 0];
    keys[i].mod  = in[i * 4 + 1];
    keys[i].code = (uint16_t)in[i * 4 + 2] | ((uint16_t)in[i * 4 + 3] << 8);
  }
}
void resetKeymapDefaults() {
  for (uint8_t i = 0; i < KEYMAP_KEYS; i++) keys[i] = defaultKeys[i];
}

// Persist current mapping to internal flash as [VERSION][36 bytes].
void saveKeymap() {
  uint8_t buf[1 + KEYMAP_LEN];
  buf[0] = KEYMAP_VERSION;
  keymapToBytes(buf + 1);
  InternalFS.remove(KEYMAP_FILE);                       // truncate / overwrite
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (f.open(KEYMAP_FILE, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    f.write(buf, sizeof(buf));
    f.close();
  }
}

// Load a saved mapping, else keep defaults. Files from older firmware keep
// their keys; the entries they did not have get their defaults.
void loadKeymap() {
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (!f.open(KEYMAP_FILE, Adafruit_LittleFS_Namespace::FILE_O_READ)) return;
  uint8_t buf[1 + KEYMAP_LEN];
  int n = f.read(buf, sizeof(buf));
  f.close();
  if (n == 1 + KEYMAP_LEN && buf[0] == KEYMAP_VERSION) bytesToKeymap(buf + 1, KEYMAP_LEN);
  else if (n == 29 && buf[0] == 3)                     bytesToKeymap(buf + 1, 28);
  else if (n == 17 && buf[0] == 2)                     bytesToKeymap(buf + 1, 16);
  else if (n == 9 && buf[0] == 1)                      bytesToKeymap(buf + 1, 8);
}

// [MOTION_FILE_VERSION][flags]. Version 2 = hold-to-tilt, tilt off by
// default; an older one-byte file (tilt on, auto-fire) is ignored.
#define MOTION_FILE_VERSION 2
void saveMotionFlags() {
  InternalFS.remove(MOTION_FILE);
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (f.open(MOTION_FILE, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    uint8_t buf[2] = { MOTION_FILE_VERSION, motionFlags };
    f.write(buf, sizeof(buf));
    f.close();
  }
}

void loadMotionFlags() {
  Adafruit_LittleFS_Namespace::File f(InternalFS);
  if (!f.open(MOTION_FILE, Adafruit_LittleFS_Namespace::FILE_O_READ)) return;
  uint8_t buf[2];
  if (f.read(buf, 2) == 2 && buf[0] == MOTION_FILE_VERSION) motionFlags = buf[1] & MOTION_ALL_BITS;
  f.close();
}

void writeMotionCharacteristic() {
  uint8_t v[2] = { motionFlags, (uint8_t)(motionPresent ? 1 : 0) };
  motionChr.write(v, sizeof(v));
}

// Bonded host switched gestures on/off: byte 0 = flags.
void motion_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl; (void) chr;
  if (len < 1) return;
  motionFlags = data[0] & MOTION_ALL_BITS;
  motionApply();
  writeMotionCharacteristic();
  motionDirty = true;
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

// Bonded host wrote the keymap characteristic (8 or 16 bytes): apply + flag for save.
void keymap_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl; (void) chr;
  if (len < 8) return;
  bytesToKeymap(data, len);
  uint8_t b[KEYMAP_LEN]; keymapToBytes(b);
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
    uint8_t b[KEYMAP_LEN]; keymapToBytes(b);
    keymapChr.write(b, sizeof(b));
    keymapDirty = true;
  } else if (data[0] == CMD_RESET_NAME) {
    setDefaultDeviceName();
    Bluefruit.setName(deviceName);
    writeDeviceNameCharacteristic();
    nameDirty = true;
    advRefreshPending = true;
  } else if (data[0] == CMD_RESET_ALL) {
    resetAllPending = true;             // flash work and reboot happen in loop()
  }
}

void setupConfigService() {
  cfgSvc.begin();

  keymapChr.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  keymapChr.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  keymapChr.setMaxLen(KEYMAP_LEN);
  keymapChr.setWriteCallback(keymap_write_cb);
  keymapChr.begin();
  uint8_t b[KEYMAP_LEN]; keymapToBytes(b);
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

  motionChr.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  motionChr.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  motionChr.setMaxLen(2);
  motionChr.setWriteCallback(motion_write_cb);
  motionChr.begin();
  writeMotionCharacteristic();
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
  // Every BLE host that subscribed to the keyboard gets the key; a link that
  // only opened the control app has no HID subscription and ignores it.
  if (Bluefruit.connected()) {
    for (uint16_t h = 0; h < BLE_MAX_CONNECTION; h++) {
      if (Bluefruit.connected(h)) blehid.keyboardReport(h, modifier, keys);
    }
    delay(KEY_TAP_MS);
    for (uint16_t h = 0; h < BLE_MAX_CONNECTION; h++) {
      if (Bluefruit.connected(h)) blehid.keyRelease(h);
    }
  }
}

// Send a media/consumer key. BLE only: the USB-HID descriptor here is keyboard-only,
// so media keys require a connected BLE host. Codes: HID_USAGE_CONSUMER_*.
void sendConsumer(uint16_t usage) {
  if (Bluefruit.connected()) {
    for (uint16_t h = 0; h < BLE_MAX_CONNECTION; h++) {
      if (Bluefruit.connected(h)) blehid.consumerKeyPress(h, usage);
    }
    delay(KEY_TAP_MS);
    for (uint16_t h = 0; h < BLE_MAX_CONNECTION; h++) {
      if (Bluefruit.connected(h)) blehid.consumerKeyRelease(h);
    }
  }
}

// Dispatch a configured button mapping to the correct HID path.
void sendMapping(const KeyMap& k) {
  if (k.type == KEYMAP_TYPE_CONSUMER) sendConsumer(k.code);
  else                                sendKey(k.mod, (uint8_t)k.code);
}

// Prints every debounced button change to the USB serial port (115200), so a
// button that does nothing can be told apart: no line = wiring/solder, a line
// but no key = mapping or host.
bool pollPressed(Button& b, const char* label) {
  bool reading = digitalRead(b.pin);
  if (reading != b.lastRead) {
    b.lastRead = reading;
    b.lastEdge = millis();
  }
  if ((millis() - b.lastEdge) > DEBOUNCE_MS && reading != b.stable) {
    b.stable = reading;
    if (Serial) Serial.printf("%s %s\n", label, b.stable == LOW ? "pressed" : "released");
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
  uint32_t raw = 0;                                    // divider enabled since boot
  for (uint8_t i = 0; i < 8; i++) raw += analogRead(PIN_VBAT);
  return (uint16_t)((float)raw / 8 * VBAT_MV_PER_LSB * VBAT_DIVIDER);
}

// LiPo resting voltage -> charge, piecewise linear. A cell spends most of its
// charge between 3.7 and 3.9 V, so a straight 3.3..4.2 V line would sit high
// for hours and then drop fast at the end.
uint8_t vbatPercent(uint16_t mv) {
  static const uint16_t curve[][2] = {
    { 4200, 100 }, { 4100, 90 }, { 4000, 79 }, { 3900, 66 }, { 3850, 57 },
    { 3800, 47 },  { 3750, 37 }, { 3700, 27 }, { 3650, 17 }, { 3600, 10 },
    { 3500, 5 },   { 3400, 2 },  { 3300, 0 },
  };
  const uint8_t n = sizeof(curve) / sizeof(curve[0]);
  if (mv >= curve[0][0]) return 100;
  for (uint8_t i = 1; i < n; i++) {
    if (mv >= curve[i][0]) {
      uint16_t v0 = curve[i][0], v1 = curve[i - 1][0];
      uint16_t p0 = curve[i][1], p1 = curve[i - 1][1];
      return (uint8_t)(p0 + (uint32_t)(mv - v0) * (p1 - p0) / (v1 - v0));
    }
  }
  return 0;
}

// Last level sent, so a little noise does not make the percentage jump about.
uint8_t batLevel = 0;
bool    batKnown = false;

// The BQ25101 charge pin (~CHG, P0.17): LOW while a battery is charging.
static inline bool charging() {
  return digitalRead(PIN_CHG_STATUS) == LOW;
}

// While USB is plugged in the divider reads the charger's output, not the cell,
// and it sags or rises with the charge current, so the percentage would wander.
// On USB the level is only allowed to go up (charging), never down; on battery
// it follows the cell, but a step up of more than a few percent (noise right
// after unplugging) is ignored.
void reportBattery(uint16_t mv) {
  uint8_t p = vbatPercent(mv);
  bool onUsb = usbPowered();
  if (batKnown) {
    if (onUsb && p < batLevel) return;                   // charger noise, not a real drop
    if (!onUsb && p > batLevel && p - batLevel < 5) return;
    if (p == batLevel || p + 1 == batLevel || p == batLevel + 1) return;
  }
  batLevel = p;
  batKnown = true;
  // write() only changes the stored value; a host that already read it (macOS
  // keeps showing that) needs a notification to see the new level.
  blebas.write(p);
  for (uint16_t h = 0; h < BLE_MAX_CONNECTION; h++) {
    if (Bluefruit.connected(h)) blebas.notify(h, p);
  }
  if (Serial) Serial.printf("battery %u mV = %u%% (%s%s)\n", mv, p,
                            onUsb ? "USB" : "battery", charging() ? ", charging" : "");
}
#endif

// Lowest-power state. Only reached on battery. Wake = button press (or, on a
// Sense with lift-to-wake on, picking it up) = reset.
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

  // Arm the buttons as wake sources. A button already held LOW (stuck,
  // pressed by the case, or a solder bridge) would wake the chip at once and
  // drain the battery in a reset loop, so it is left out.
  const uint8_t wakePins[] = { BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_L1_PIN, BUTTON_R1_PIN };
  for (uint8_t pin : wakePins) {
    if (digitalRead(pin) == HIGH) pinMode(pin, INPUT_PULLUP_SENSE);
  }
  motionPrepareSleep();                   // Sense: movement wakes it too

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

  // The config service in the scan response lets the control app list only
  // Clawds, and find one that the computer already holds as a keyboard.
  Bluefruit.ScanResponse.clearData();
  Bluefruit.ScanResponse.addService(cfgSvc);

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
  advStart = millis();
}

// ----------------------------- BLE callbacks --------------------------------

// Keep advertising while a link is free, so the control app (or a second
// computer) finds the keyboard without "Forget"-ing it on the first host.
void connect_callback(uint16_t conn_handle) {
  (void) conn_handle;
  lastActivity = millis();
#if ENABLE_BATTERY
  batKnown = false;                     // resend the level (as a notification) soon
  lastBat = millis() - BATTERY_PERIOD_MS + 3000;
#endif
  if (Bluefruit.Periph.connected() < BLE_LINKS) Bluefruit.Advertising.start(0);
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
// Print what is saved in flash and in use (serial monitor, 115200).
void printSettings() {
  if (!Serial) return;
  static const char* names[KEYMAP_KEYS] = {
    "B1", "B2", "L1", "R1", "TAP", "TILT left", "TILT right", "TILT fwd", "TILT back",
  };
  Serial.println("---- settings ----");
  Serial.printf("name: %s\n", deviceName);
  for (uint8_t i = 0; i < KEYMAP_KEYS; i++) {
    const KeyMap& k = keys[i];
    bool isDefault = k.type == defaultKeys[i].type && k.mod == defaultKeys[i].mod && k.code == defaultKeys[i].code;
    Serial.printf("%-10s %s mod=0x%02X code=0x%02X%s\n", names[i],
                  k.type == KEYMAP_TYPE_CONSUMER ? "media   " : "keyboard",
                  k.mod, k.code, isDefault ? "" : "  (changed)");
  }
  Serial.printf("motion sensor: %s, wake=%d tap=%d tilt L/R=%d tilt F/B=%d\n",
                motionPresent ? "found" : "none",
                !!(motionFlags & MOTION_WAKE_BIT), !!(motionFlags & MOTION_TAP_BIT),
                !!(motionFlags & MOTION_TILT_BIT), !!(motionFlags & MOTION_PITCH_BIT));
  Serial.println("send 's' to print this again, 'R' to wipe settings (keeps bonds)");
  Serial.println("------------------");
}

// Factory settings: delete the keymap, name and gesture files and go back to
// the compiled defaults. Bluetooth bonds are kept (B1 + B2 at boot wipes those).
void resetAllSettings() {
  InternalFS.remove(KEYMAP_FILE);
  InternalFS.remove(NAME_FILE);
  InternalFS.remove(MOTION_FILE);
  resetKeymapDefaults();
  setDefaultDeviceName();
  motionFlags = MOTION_WAKE_BIT | MOTION_TAP_BIT;
  if (Serial) Serial.println("settings wiped: defaults restored, rebooting");
  delay(100);
  NVIC_SystemReset();                   // reboot so name, USB and BLE start clean
}

// Hold L1 + R1 at boot for SETTINGS_RESET_HOLD_MS to wipe the saved settings
// (key mapping, name, gestures). Blue while holding, 3 blue blinks = done.
void maybeResetSettings() {
  if (digitalRead(BUTTON_L1_PIN) != LOW || digitalRead(BUTTON_R1_PIN) != LOW) return;
  uint32_t t = millis();
  while (digitalRead(BUTTON_L1_PIN) == LOW && digitalRead(BUTTON_R1_PIN) == LOW) {
    setRGB(false, false, true);
    if ((millis() - t) >= SETTINGS_RESET_HOLD_MS) {
      for (int i = 0; i < 3; i++) {
        setRGB(false, false, true); delay(120);
        setRGB(false, false, false); delay(120);
      }
      resetAllSettings();               // reboots
    }
    delay(20);
  }
  setRGB(false, false, false);
}

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
  pinMode(BUTTON_L1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_R1_PIN, INPUT_PULLUP);
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE,  OUTPUT);
  setRGB(false, false, false);

  pinMode(LED_USER_PIN, OUTPUT);          // BLE-controlled external LED
  applyUserLed(LED_USER_DEFAULT);

  // Charge current select. Seeed: 50 mA = P0.13 high impedance (input, no
  // pull), 100 mA = output LOW. Driving it HIGH is not one of the two.
  if (CHARGE_100MA) {
    pinMode(PIN_CHARGING_CURRENT, OUTPUT);
    digitalWrite(PIN_CHARGING_CURRENT, LOW);
  } else {
    pinMode(PIN_CHARGING_CURRENT, INPUT);
  }

#if ENABLE_BATTERY
  // Keep the battery-sense divider enabled (P0.14 LOW) and configure the ADC.
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  pinMode(PIN_CHG_STATUS, INPUT_PULLUP);  // open-drain charge status
  analogReference(AR_DEFAULT);          // 3.6 V full scale
  analogReadResolution(12);
#endif

  // Load any saved key mapping from internal flash (keeps compiled defaults if none).
  resetKeymapDefaults();
  loadKeymap();
  loadMotionFlags();
  motionBegin();                          // Sense only; false and harmless elsewhere

  // BLE is always available (primary, or fallback when USB is unplugged).
  // A larger MTU (128) lets the 36-byte keymap go in one write instead of a
  // split long write.
  Bluefruit.configPrphBandwidth(BANDWIDTH_HIGH);
  Bluefruit.begin(BLE_LINKS, 0);
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

  // Boot-time gestures: hold B1 + B2 ~5 s to wipe all stored bonds; hold
  // L1 + R1 ~5 s to wipe the saved settings (key mapping, name, gestures).
  maybeResetBonds();
  maybeResetSettings();

  bledis.setManufacturer("Seeed Studio");
  bledis.setModel("XIAO nRF52840");
  bledis.begin();

  blehid.begin();
#if ENABLE_BATTERY
  blebas.begin();
  reportBattery(readVbatMv());          // real level from the start, not a fixed 100%
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

  bool pressed[KEYMAP_KEYS] = {
    pollPressed(btn1, "B1"), pollPressed(btn2, "B2"),
    pollPressed(btnL1, "L1"), pollPressed(btnR1, "R1"),
    false, false, false, false, false,
  };
  // On a Sense with tilt on, holding L1 + R1 together turns on tilt: while
  // both are held, leaning Clawd sends the arrow keys (see motion.h) and the
  // shoulders send nothing themselves. To tell the chord from a single press,
  // L1/R1 keys go out SHOULDER_CHORD_MS late (only when tilt is on).
  static bool l1Pending = false, r1Pending = false, chord = false;
  static uint32_t l1At = 0, r1At = 0;
  bool tiltOn = motionPresent && (motionFlags & (MOTION_TILT_BIT | MOTION_PITCH_BIT));
  bool l1Down = btnL1.stable == LOW, r1Down = btnR1.stable == LOW;
  if (tiltOn) {
    if (pressed[2]) { l1Pending = true; l1At = now; pressed[2] = false; }
    if (pressed[3]) { r1Pending = true; r1At = now; pressed[3] = false; }
    if (l1Down && r1Down && (l1Pending || r1Pending || chord)) {
      if (!chord) {
        chord = true;
        if (Serial) Serial.println("L1+R1 held: tilt on");
      }
      l1Pending = r1Pending = false;
    } else if (chord && !l1Down && !r1Down) {
      chord = false;                    // both released: tilt off, no key sent
      if (Serial) Serial.println("tilt off");
    } else if (!chord) {
      if (l1Pending && now - l1At > SHOULDER_CHORD_MS) { l1Pending = false; pressed[2] = true; }
      if (r1Pending && now - r1At > SHOULDER_CHORD_MS) { r1Pending = false; pressed[3] = true; }
    }
    if (chord) lastActivity = now;      // do not sleep while tilting
  } else {
    chord = false;
  }
  motionTiltHeld = chord;
  switch (motionPoll((bool)Serial)) {
    case MOTION_DOUBLE_TAP: pressed[KEY_TAP] = true;        if (Serial) Serial.println("TAP double"); break;
    case MOTION_TILT_LEFT:  pressed[KEY_TILT_LEFT] = true;  if (Serial) Serial.println("TILT left");  break;
    case MOTION_TILT_RIGHT: pressed[KEY_TILT_RIGHT] = true; if (Serial) Serial.println("TILT right"); break;
    case MOTION_TILT_FORWARD: pressed[KEY_TILT_FWD] = true; if (Serial) Serial.println("TILT forward"); break;
    case MOTION_TILT_BACK:  pressed[KEY_TILT_BACK] = true;  if (Serial) Serial.println("TILT back");  break;
    default: break;
  }

  if (active) {
    for (uint8_t i = 0; i < KEYMAP_KEYS; i++) {
      if (!pressed[i]) continue;
      sendMapping(keys[i]);
      ledFlashUntil = now + LED_FLASH_MS;
      lastActivity = now;
    }
  }

  // Persist changed settings outside BLE callbacks (flash writes are blocking).
  if (resetAllPending) resetAllSettings();
  // Serial commands: 's' prints the saved settings, 'R' wipes them.
  // The settings are also printed once, a few seconds after the monitor opens.
  static bool printedSettings = false;
  if (Serial) {
    static uint32_t serialSince = 0;
    if (!serialSince) serialSince = now;
    if (!printedSettings && now - serialSince > 2000) { printedSettings = true; printSettings(); }
    while (Serial.available()) {
      int c = Serial.read();
      if (c == 's') printSettings();
      else if (c == 'R') resetAllSettings();
    }
  } else {
    printedSettings = false;
  }
  if (keymapDirty) { keymapDirty = false; saveKeymap(); }
  if (motionDirty) { motionDirty = false; saveMotionFlags(); }
  if (nameDirty) { nameDirty = false; saveDeviceName(); }
  if (advRefreshPending && !Bluefruit.connected()) { advRefreshPending = false; startAdv(); }
  // The library only restarts advertising when the last link drops; reopen
  // the free slot when one of two links goes away.
  uint8_t links = Bluefruit.Periph.connected();
  if (links > 0 && links < BLE_LINKS && !Bluefruit.Advertising.isRunning()) {
    Bluefruit.Advertising.start(0);
  }

#if ENABLE_BATTERY
  if (now - lastBat > BATTERY_PERIOD_MS) {
    lastBat = now;
    uint16_t mv = readVbatMv();
    reportBattery(mv);
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

  // Raw pin levels every 2 s on the serial port (1 = released, 0 = pressed or
  // shorted to GND). A pin stuck at 0 without a press is a solder bridge.
  static uint32_t lastRaw = 0, lastPower = 0;
#if ENABLE_BATTERY
  // Power line every 10 s on the serial port: what the battery path reads.
  if (Serial && now - lastPower > 10000) {
    lastPower = now;
    uint16_t mv = readVbatMv();
    Serial.printf("power %s, charger %s, VBAT %u mV (%u%%), sent %u%%\n",
                  usbPowered() ? "USB" : "battery",
                  charging() ? "CHARGING" : "idle/full/no battery",
                  mv, vbatPercent(mv), batLevel);
  }
#endif
  if (Serial && now - lastRaw > 2000) {
    lastRaw = now;
    Serial.printf("raw B1=%d B2=%d L1=%d R1=%d\n",
                  digitalRead(BUTTON_1_PIN), digitalRead(BUTTON_2_PIN),
                  digitalRead(BUTTON_L1_PIN), digitalRead(BUTTON_R1_PIN));
#ifdef DIAG_ANALOG
    // TEMP diagnosis: voltage on D4/D5 against the internal ~13k pull-up.
    // ~0 mV = hard short to GND, 300..2500 mV = leak (flux, partial bridge).
    analogReference(AR_DEFAULT);
    analogReadResolution(12);
    uint32_t a4 = analogRead(A4), a5 = analogRead(A5), a0 = analogRead(A0);
    pinMode(BUTTON_L1_PIN, INPUT_PULLUP);
    pinMode(BUTTON_R1_PIN, INPUT_PULLUP);
    pinMode(BUTTON_1_PIN, INPUT_PULLUP);
    Serial.printf("mV D0=%lu D4=%lu D5=%lu\n",
                  (unsigned long)(a0 * 3600 / 4096), (unsigned long)(a4 * 3600 / 4096),
                  (unsigned long)(a5 * 3600 / 4096));
#endif
  }

  updateLed(active);
  delay(5);
}
