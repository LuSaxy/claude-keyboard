// Motion sensor support for the XIAO nRF52840 Sense (LSM6DS3TR-C on its own
// I2C bus, Wire1). Three gestures, each switched on/off at run time:
//   - lift to wake: in deep sleep the accelerometer keeps running in low power
//     and its wake-up interrupt (INT1) wakes the nRF52840, so picking Clawd up
//     connects it before the first key press;
//   - double tap on the case;
//   - tilt (hold L1 + R1): while both shoulder buttons are held, leaning
//     Clawd sends the arrow keys like a joystick: the pose when the buttons
//     went down is "level", a lean past MOTION_TILT_DEG sends the key at once
//     and then repeats it, faster the further it leans. Release the buttons
//     and tilt does nothing, so it never fires by accident.
// Built only for the Sense boards; elsewhere motionBegin() returns false and
// nothing here touches a pin. The sensor is also checked at run time
// (WHO_AM_I), so a missing sensor just disables the gestures.
#pragma once

#if defined(ARDUINO_Seeed_XIAO_nRF52840_Sense) || defined(ARDUINO_Seeed_XIAO_nRF52840_Sense_Plus)
#define HAS_MOTION_SENSOR 1
#include <Wire.h>
#else
#define HAS_MOTION_SENSOR 0
#endif

#ifndef MOTION_WAKE_THS
#define MOTION_WAKE_THS 3      // wake threshold, 1 LSB = 31 mg (2 g / 64): 3 = ~94 mg
#endif
#ifndef MOTION_TAP_THS
#define MOTION_TAP_THS 12      // tap threshold, 1 LSB = 62.5 mg (2 g / 32): 12 = 0.75 g
#endif
#ifndef MOTION_TILT_DEG
#define MOTION_TILT_DEG 15     // lean from the start pose that sends a key
#endif
#ifndef MOTION_TILT_FAST_DEG
#define MOTION_TILT_FAST_DEG 40 // lean where the repeat reaches its fastest rate
#endif
#ifndef MOTION_REPEAT_SLOW_MS
#define MOTION_REPEAT_SLOW_MS 400 // repeat interval just past MOTION_TILT_DEG
#endif
#ifndef MOTION_REPEAT_FAST_MS
#define MOTION_REPEAT_FAST_MS 60  // repeat interval at MOTION_TILT_FAST_DEG and beyond
#endif
// Which sensor axis points to Clawd's right, and the signs. The module sits
// upside down under the PCB, rotated. Measured on the first Sense board:
// leaning right raises sensor X, leaning forward (top away from you) lowers
// sensor Y. Check with the serial "tilt" line: right must give roll > 0,
// forward pitch > 0. Swap the axis or flip a sign if a board differs.
#ifndef MOTION_TILT_AXIS
#define MOTION_TILT_AXIS 0     // sensor axis for left/right: 0 = X, 1 = Y (the other is forward/back)
#endif
#ifndef MOTION_TILT_SIGN
#define MOTION_TILT_SIGN 1     // 1 or -1: roll sign
#endif
#ifndef MOTION_PITCH_SIGN
#define MOTION_PITCH_SIGN -1   // 1 or -1: pitch sign
#endif

#define MOTION_WAKE_BIT 0x01
#define MOTION_TAP_BIT  0x02
#define MOTION_TILT_BIT 0x04   // tilt left / right
#define MOTION_PITCH_BIT 0x08  // tilt forward / back
#define MOTION_ALL_BITS (MOTION_WAKE_BIT | MOTION_TAP_BIT | MOTION_TILT_BIT | MOTION_PITCH_BIT)

enum MotionEvent {
  MOTION_NONE, MOTION_DOUBLE_TAP,
  MOTION_TILT_LEFT, MOTION_TILT_RIGHT, MOTION_TILT_FORWARD, MOTION_TILT_BACK,
};

bool    motionPresent = false;
// Defaults: lift to wake and double tap on, tilt off (turn it on in the app).
uint8_t motionFlags   = MOTION_WAKE_BIT | MOTION_TAP_BIT;
bool    motionTiltHeld = false;   // set by the firmware while L1 + R1 are held

#if HAS_MOTION_SENSOR

namespace lsm {
const uint8_t ADDR        = 0x6A;  // SA0 = 0 on the XIAO Sense
const uint8_t WHO_AM_I    = 0x0F;  // reads 0x6A
const uint8_t CTRL1_XL    = 0x10;
const uint8_t CTRL3_C     = 0x12;
const uint8_t CTRL6_C     = 0x15;
const uint8_t WAKE_UP_SRC = 0x1B;
const uint8_t TAP_SRC     = 0x1C;
const uint8_t OUTX_L_XL   = 0x28;
const uint8_t TAP_CFG     = 0x58;
const uint8_t TAP_THS_6D  = 0x59;
const uint8_t INT_DUR2    = 0x5A;
const uint8_t WAKE_UP_THS = 0x5B;
const uint8_t WAKE_UP_DUR = 0x5C;
const uint8_t MD1_CFG     = 0x5E;

bool write(uint8_t reg, uint8_t val) {
  Wire1.beginTransmission(ADDR);
  Wire1.write(reg);
  Wire1.write(val);
  return Wire1.endTransmission() == 0;
}

int read(uint8_t reg) {
  Wire1.beginTransmission(ADDR);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return -1;
  if (Wire1.requestFrom(ADDR, (uint8_t)1) != 1) return -1;
  return Wire1.read();
}

bool readAccel(int16_t out[3]) {
  Wire1.beginTransmission(ADDR);
  Wire1.write(OUTX_L_XL);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom(ADDR, (uint8_t)6) != 6) return false;
  for (int i = 0; i < 3; i++) {
    uint8_t lo = Wire1.read(), hi = Wire1.read();
    out[i] = (int16_t)(lo | (hi << 8));
  }
  return true;
}
}  // namespace lsm

// Route the awake interrupts to INT1 according to motionFlags.
void motionApply() {
  if (!motionPresent) return;
  lsm::write(lsm::MD1_CFG, (motionFlags & MOTION_TAP_BIT) ? 0x08 : 0x00);  // double tap
  lsm::read(lsm::TAP_SRC);
}

// Power the sensor (own supply pin, high drive), check it and configure it.
bool motionBegin() {
  pinMode(PIN_LSM6DS3TR_C_POWER, OUTPUT);
  uint32_t p = g_ADigitalPinMap[PIN_LSM6DS3TR_C_POWER];
  NRF_GPIO_Type* port = (p >= 32) ? NRF_P1 : NRF_P0;
  port->PIN_CNF[p & 31] = (port->PIN_CNF[p & 31] & ~GPIO_PIN_CNF_DRIVE_Msk)
                        | (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos);
  digitalWrite(PIN_LSM6DS3TR_C_POWER, HIGH);
  delay(40);                                   // sensor boot time

  Wire1.begin();
  Wire1.setClock(400000);
  if (lsm::read(lsm::WHO_AM_I) != 0x6A) {
    digitalWrite(PIN_LSM6DS3TR_C_POWER, LOW);  // no sensor: do not waste power
    motionPresent = false;
    return false;
  }
  lsm::write(lsm::CTRL3_C, 0x01);              // software reset
  delay(5);
  lsm::write(lsm::CTRL3_C, 0x44);              // BDU + auto-increment, INT push-pull active high

  // Awake: 416 Hz, +-2 g; double tap on any axis, latched until TAP_SRC is read.
  lsm::write(lsm::CTRL1_XL, 0x60);
  lsm::write(lsm::CTRL6_C, 0x00);              // high-performance mode
  lsm::write(lsm::TAP_CFG, 0x8F);              // interrupts on, tap x/y/z, latch
  lsm::write(lsm::TAP_THS_6D, MOTION_TAP_THS & 0x1F);
  lsm::write(lsm::INT_DUR2, 0x7F);             // tap window (dur 7, quiet 3, shock 3)
  lsm::write(lsm::WAKE_UP_THS, 0x80);          // single + double tap
  lsm::write(lsm::WAKE_UP_DUR, 0x00);
  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT);
  motionPresent = true;
  motionApply();
  return true;
}

// Polled from loop(). Returns at most one event per call.
MotionEvent motionPoll(bool debug) {
  if (!motionPresent) return MOTION_NONE;

  if ((motionFlags & MOTION_TAP_BIT) && digitalRead(PIN_LSM6DS3TR_C_INT1) == HIGH) {
    int src = lsm::read(lsm::TAP_SRC);         // reading clears the latch
    if (src >= 0 && (src & 0x10)) return MOTION_DOUBLE_TAP;
  }

  if (!(motionFlags & (MOTION_TILT_BIT | MOTION_PITCH_BIT))) return MOTION_NONE;
  static bool wasHeld = false, started = false;
  static float rest[2] = { 0, 0 };
  static MotionEvent lastDir = MOTION_NONE;
  static uint32_t lastRead = 0, lastSent = 0, lastDebug = 0;
  if (!motionTiltHeld) {                         // tilt only works while held
    wasHeld = started = false;
    lastDir = MOTION_NONE;
    return MOTION_NONE;
  }
  uint32_t now = millis();
  if (now - lastRead < 20) return MOTION_NONE;  // 50 Hz is plenty for tilting
  lastRead = now;

  int16_t a[3];
  if (!lsm::readAccel(a)) return MOTION_NONE;
  float x = a[0], y = a[1], z = a[2];
  float g = sqrtf(x * x + y * y + z * z);
  const float oneG = 16393.0f;                 // 0.061 mg/LSB at +-2 g
  if (g < 0.7f * oneG || g > 1.3f * oneG) return MOTION_NONE;  // shaken: ignore
  float sx = (MOTION_TILT_AXIS == 0 ? x : y) / g;   // sin(roll), before sign
  float sy = (MOTION_TILT_AXIS == 0 ? y : x) / g;   // sin(pitch)
  float ang[2] = {
    asinf(sx) * 57.2958f * MOTION_TILT_SIGN,       // roll
    asinf(sy) * 57.2958f * MOTION_PITCH_SIGN,      // pitch
  };

  // The pose when the buttons went down is "level".
  if (!wasHeld) {
    wasHeld = true;
    started = false;
    lastDir = MOTION_NONE;
  }
  if (!started) {
    rest[0] = ang[0]; rest[1] = ang[1];
    started = true;
    if (debug) Serial.printf("tilt start, level = %.0f/%.0f\n", rest[0], rest[1]);
    return MOTION_NONE;
  }
  float rel[2] = { ang[0] - rest[0], ang[1] - rest[1] };
  if (!(motionFlags & MOTION_TILT_BIT))  rel[0] = 0;
  if (!(motionFlags & MOTION_PITCH_BIT)) rel[1] = 0;

  if (debug && now - lastDebug > 300) {
    lastDebug = now;
    Serial.printf("tilt %+.0f/%+.0f deg from start\n", rel[0], rel[1]);
  }

  // The larger lean wins, so a diagonal gives one clear direction.
  bool rollWins = fabsf(rel[0]) >= fabsf(rel[1]);
  float lean = rollWins ? fabsf(rel[0]) : fabsf(rel[1]);
  if (lean < MOTION_TILT_DEG) {                  // back near level: stop
    lastDir = MOTION_NONE;
    return MOTION_NONE;
  }
  MotionEvent dir = rollWins ? (rel[0] > 0 ? MOTION_TILT_RIGHT : MOTION_TILT_LEFT)
                             : (rel[1] > 0 ? MOTION_TILT_FORWARD : MOTION_TILT_BACK);
  // Repeat rate: slow just past the threshold, fast at MOTION_TILT_FAST_DEG.
  float k = (lean - MOTION_TILT_DEG) / (float)(MOTION_TILT_FAST_DEG - MOTION_TILT_DEG);
  if (k > 1) k = 1;
  uint32_t interval = MOTION_REPEAT_SLOW_MS - (uint32_t)(k * (MOTION_REPEAT_SLOW_MS - MOTION_REPEAT_FAST_MS));
  if (dir != lastDir || now - lastSent >= interval) {
    lastDir = dir;
    lastSent = now;
    return dir;
  }
  return MOTION_NONE;
}

// Called right before System OFF. Returns true when INT1 is armed as a wake
// source; false (sensor off) if lift-to-wake is disabled or the line is stuck.
bool motionPrepareSleep() {
  if (!motionPresent) return false;
  if (!(motionFlags & MOTION_WAKE_BIT)) {
    digitalWrite(PIN_LSM6DS3TR_C_POWER, LOW);  // nothing to do while asleep
    return false;
  }
  // Low-power 26 Hz, only the wake-up (movement) interrupt on INT1.
  lsm::write(lsm::MD1_CFG, 0x00);
  lsm::write(lsm::CTRL1_XL, 0x20);             // 26 Hz, +-2 g
  lsm::write(lsm::CTRL6_C, 0x10);              // XL_HM_MODE = 1: low power
  lsm::write(lsm::TAP_CFG, 0x81);              // interrupts on, slope filter, latch
  lsm::write(lsm::WAKE_UP_DUR, 0x00);
  lsm::write(lsm::WAKE_UP_THS, MOTION_WAKE_THS & 0x3F);
  delay(100);                                  // let the slope filter settle
  lsm::read(lsm::WAKE_UP_SRC);                 // clear the latch
  lsm::read(lsm::TAP_SRC);
  lsm::write(lsm::MD1_CFG, 0x20);              // wake-up -> INT1
  delay(2);
  if (digitalRead(PIN_LSM6DS3TR_C_INT1) == HIGH) {
    lsm::read(lsm::WAKE_UP_SRC);
    delay(2);
    if (digitalRead(PIN_LSM6DS3TR_C_INT1) == HIGH) {
      // Still moving or stuck: do not arm it, or Clawd would wake at once.
      digitalWrite(PIN_LSM6DS3TR_C_POWER, LOW);
      return false;
    }
  }
  // The power pin keeps its level in System OFF, so the sensor keeps running.
  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT_SENSE_HIGH);
  return true;
}

#else

void motionApply() {}
bool motionBegin() { return false; }
MotionEvent motionPoll(bool) { return MOTION_NONE; }
bool motionPrepareSleep() { return false; }

#endif
