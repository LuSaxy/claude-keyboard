/*
 * Clawd self-test: checks that the XIAO nRF52840 (or Plus) is soldered
 * correctly onto the Clawd V2 board, then lets you press every button.
 *
 * Flash it, open the serial monitor at 115200 and read the report:
 *   - every used pin: level with the internal pull-up and what it means
 *   - pins stuck to GND or to a supply
 *   - bridges: pins joined to each other (solder bridges)
 *   - battery voltage
 *   - then press B1, B2, L1, R1 and flip MODE: each change is printed
 * The report repeats every 20 s (or send "t"), so you can watch it while
 * reworking. Flash the keyboard firmware again when everything is OK.
 *
 * Safety: a pin is only driven HIGH when it reads clean HIGH with its
 * pull-up (nothing holds it low), or when it is D3 (the eye LEDs). Pins on
 * the battery divider and the NFC pads are only read.
 */
#include <Adafruit_TinyUSB.h>

struct TestPin {
  uint8_t pin;
  const char* name;
  const char* role;
  int8_t adc;       // analog channel or -1
  bool drivable;    // may be driven HIGH for the bridge test
};

TestPin pins[] = {
  { D0,  "D0",  "B1 key",      A0, true  },
  { D1,  "D1",  "B2 key",      A1, true  },
  { D2,  "D2",  "MODE switch", A2, true  },
  { D3,  "D3",  "eye LEDs",    A3, true  },
  { D4,  "D4",  "L1 shoulder", A4, true  },
  { D5,  "D5",  "R1 shoulder", A5, true  },
  { D6,  "D6",  "unused",      -1, true  },
  { D7,  "D7",  "unused",      -1, true  },
  { D8,  "D8",  "unused",      -1, true  },
  { D9,  "D9",  "unused",      -1, true  },
  { D10, "D10", "unused",      -1, true  },
#if defined(ARDUINO_Seeed_XIAO_nRF52840_Plus) || defined(ARDUINO_Seeed_XIAO_nRF52840_Sense_Plus)
  // Extra 1.27 mm pads of the Plus, between the normal castellations.
  { D11, "D11", "Plus pad (P0.15)",     -1, true  },
  { D12, "D12", "Plus pad (P0.19)",     -1, true  },
  { D13, "D13", "Plus pad (P1.01)",     -1, true  },
  { D14, "D14", "Plus pad NFC1",        -1, false },
  { D15, "D15", "Plus pad NFC2",        -1, false },
  { D17, "D17", "Plus pad (P1.03)",     -1, true  },
  { D18, "D18", "Plus pad (P1.05)",     -1, true  },
  { D19, "D19", "Plus pad (P1.07)",     -1, true  },
#endif
};
const uint8_t N = sizeof(pins) / sizeof(pins[0]);

bool passed = false;

uint32_t mv(int8_t adc) { return adc < 0 ? 0 : analogRead(adc) * 3600UL / 4096; }

void allMode(uint32_t mode) {
  for (uint8_t i = 0; i < N; i++) pinMode(pins[i].pin, mode);
}

uint32_t vbatMv() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  delay(5);
  uint32_t raw = 0;
  for (int k = 0; k < 8; k++) raw += analogRead(PIN_VBAT);
  return (uint32_t)((raw / 8) * 3600.0f / 4096.0f * 2.9608f);
}

uint32_t vbatPinMv() {  // the divider pin itself, used to spot bridges to it
  uint32_t raw = 0;
  for (int k = 0; k < 4; k++) raw += analogRead(PIN_VBAT);
  return (raw / 4) * 3600UL / 4096;
}

void report() {
  uint8_t problems = 0;
  Serial.println();
  Serial.println("================ CLAWD SELF-TEST ================");
#if defined(ARDUINO_Seeed_XIAO_nRF52840_Sense_Plus)
  Serial.println("Board: XIAO nRF52840 Sense Plus (extra pads tested too)");
#elif defined(ARDUINO_Seeed_XIAO_nRF52840_Plus)
  Serial.println("Board: XIAO nRF52840 Plus (extra pads tested too)");
#elif defined(ARDUINO_Seeed_XIAO_nRF52840_Sense)
  Serial.println("Board: XIAO nRF52840 Sense");
#else
  Serial.println("Board: XIAO nRF52840");
#endif
  Serial.println("Do not press any button during this part.");
  Serial.println();

  // 1. Every pin with its pull-up: who holds it low?
  allMode(INPUT_PULLUP);
  delay(20);
  bool clean[N];
  bool leaky[N];
  Serial.println("-- 1. Levels with pull-up --");
  for (uint8_t i = 0; i < N; i++) {
    TestPin& p = pins[i];
    bool hi = digitalRead(p.pin);
    uint32_t v = mv(p.adc);
    clean[i] = hi && (p.adc < 0 || v > 2800);
    leaky[i] = false;
    const char* verdict = "OK";
    const char* note = "";
    if (p.pin == D2) {
      note = hi ? "MODE = BLE position" : "MODE = USB position";
    } else if (p.pin == D3) {
      if (v > 900 && v < 2600) note = "LED load, as expected";
      else if (v >= 2600) { verdict = "WARN"; note = "no LED current: check R1/R2, D1/D2"; }
      else { verdict = "FAIL"; note = "held near 0 V: short to GND"; problems++; }
    } else if (p.adc >= 0 && !clean[i]) {
      verdict = "FAIL";
      note = v < 300 ? "stuck LOW: short to GND, or button held"
                     : "pulled down halfway: leak to GND or a bridge (see 3.)";
      leaky[i] = v >= 300;
      problems++;
    } else if (p.adc < 0 && !hi) {
      verdict = "FAIL"; note = "stuck LOW: bridged or shorted"; problems++;
    }
    if (p.adc >= 0) Serial.printf("[%-4s] %-3s %-16s %4lu mV  %s\n", verdict, p.name, p.role, (unsigned long)v, note);
    else            Serial.printf("[%-4s] %-3s %-16s  %s     %s\n", verdict, p.name, p.role, hi ? "HIGH" : "LOW ", note);
  }

  // 2. Pull-down: anything tied to a supply?
  allMode(INPUT_PULLDOWN);
  delay(20);
  Serial.println();
  Serial.println("-- 2. Tied to 3V3 / battery? --");
  bool anySupply = false;
  for (uint8_t i = 0; i < N; i++) {
    if (digitalRead(pins[i].pin)) {
      Serial.printf("[FAIL] %s reads HIGH with pull-down: tied to a supply\n", pins[i].name);
      clean[i] = false;
      anySupply = true;
      problems++;
    }
  }
  if (!anySupply) Serial.println("[OK  ] none");

  // 3. Bridges: drive one pin HIGH, all others pulled down; who follows?
  Serial.println();
  Serial.println("-- 3. Bridges between pins --");
  bool anyBridge = false;
  for (uint8_t i = 0; i < N; i++) {
    bool canDrive = pins[i].drivable && (clean[i] || pins[i].pin == D3);
    if (!canDrive) continue;
    allMode(INPUT_PULLDOWN);
    uint32_t vbatBefore = vbatPinMv();
    pinMode(pins[i].pin, OUTPUT);
    digitalWrite(pins[i].pin, HIGH);
    delay(5);
    char list[160] = "";
    for (uint8_t j = 0; j < N; j++) {
      if (j == i) continue;
      if (digitalRead(pins[j].pin)) {
        strncat(list, " ", sizeof(list) - strlen(list) - 1);
        strncat(list, pins[j].name, sizeof(list) - strlen(list) - 1);
      }
    }
    uint32_t vbatAfter = vbatPinMv();
    if (vbatAfter > vbatBefore + 400) strncat(list, " VBAT-pad", sizeof(list) - strlen(list) - 1);
    digitalWrite(pins[i].pin, LOW);
    pinMode(pins[i].pin, INPUT_PULLDOWN);
    if (list[0]) {
      Serial.printf("[FAIL] %s is joined to:%s\n", pins[i].name, list);
      anyBridge = true;
      problems++;
    }
  }
  if (!anyBridge) Serial.println("[OK  ] no bridges found");
  for (uint8_t i = 0; i < N; i++) {
    if (leaky[i] && !anyBridge) {
      Serial.printf("[HINT] %s is halfway but bridged to nothing: a resistive leak to GND.\n"
                    "       Clean flux (IPA) around the module and the %s parts; if it stays,\n"
                    "       lift one leg of its switch to tell switch from board.\n",
                    pins[i].name, pins[i].role);
    }
  }

  // 4. Battery
  allMode(INPUT_PULLUP);
  Serial.println();
  Serial.println("-- 4. Battery --");
  uint32_t vb = vbatMv();
  Serial.printf("VBAT = %lu mV  ", (unsigned long)vb);
  if (vb > 3300 && vb < 4400) Serial.println("(a cell or the charger is present)");
  else if (vb >= 4400)        Serial.println("(too high: check the reading)");
  else                        Serial.println("(no battery seen: POWER off, or BAT+/BAT- not soldered)");
  Serial.println("Real check: POWER = ON, unplug USB. If the eyes keep blinking, the battery path works.");

  passed = problems == 0;
  Serial.println();
  if (problems == 0) Serial.println("RESULT: PASS. Now press B1, B2, L1, R1 and flip MODE.");
  else               Serial.printf("RESULT: FAIL (%u problem%s). Fix the joints above, the test repeats every 20 s.\n",
                                   problems, problems == 1 ? "" : "s");
  Serial.println("=================================================");
  allMode(INPUT_PULLUP);
}

struct Watch { uint8_t pin; const char* label; const char* low; const char* high; bool last; };
Watch watch[] = {
  { D0, "B1",   "pressed", "released", true },
  { D1, "B2",   "pressed", "released", true },
  { D4, "L1",   "pressed", "released", true },
  { D5, "R1",   "pressed", "released", true },
  { D2, "MODE", "USB",     "BLE",      true },
};

uint32_t lastReport = 0;
bool eyes = false;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 10000) delay(10);
  delay(300);
  analogReference(AR_DEFAULT);
  analogReadResolution(12);
  pinMode(LED_RED, OUTPUT);   digitalWrite(LED_RED, HIGH);
  pinMode(LED_GREEN, OUTPUT); digitalWrite(LED_GREEN, HIGH);
  pinMode(LED_BLUE, OUTPUT);  digitalWrite(LED_BLUE, HIGH);
  report();
  for (auto& w : watch) w.last = digitalRead(w.pin);
  lastReport = millis();
}

void loop() {
  for (auto& w : watch) {
    bool v = digitalRead(w.pin);
    if (v != w.last) {
      delay(15);
      v = digitalRead(w.pin);
      if (v != w.last) {
        w.last = v;
        Serial.printf("%s %s\n", w.label, v ? w.high : w.low);
        digitalWrite(LED_GREEN, v ? HIGH : LOW);
      }
    }
  }

  // Heartbeat: eye LEDs blink once a second after a PASS (visible on battery
  // too). Not while failing: a pin bridged to D3 would toggle with it.
  static uint32_t lastBlink = 0;
  if (passed && millis() - lastBlink > 500) {
    lastBlink = millis();
    eyes = !eyes;
    pinMode(D3, OUTPUT);
    digitalWrite(D3, eyes);
  }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't' || c == 'T') { report(); lastReport = millis(); }
  }
  if (millis() - lastReport > 20000) {
    report();
    for (auto& w : watch) w.last = digitalRead(w.pin);
    lastReport = millis();
  }
  delay(2);
}
