/* Homework #3 – Home Alarm System (Dual LED version, no EEPROM)

   This program implements a non-blocking home alarm system using an Arduino.
   It features two main operational modes, selectable via the 'RUN_TEST_MODE' constant:
   
   --- HARDWARE TEST MODE (RUN_TEST_MODE > 0) ---
   If 'RUN_TEST_MODE' is set to a value from 1 to 4, the main alarm logic in
   loop() is skipped. Instead, a specific, isolated hardware test is run.
   - 1: Test Red/Green LEDs (cycles Red, Green, Both)
   - 2: Test Buzzer (toggles on/off)
   - 3: Test Ultrasonic Sensor (prints distance to Serial)
   - 4: Test LDR Sensor (prints analog value to Serial)

   --- NORMAL ALARM FLOW (RUN_TEST_MODE = 0) ---
   (Flow is identical to the RGB version, but using separate LEDs)
   1. Startup & Baseline: 'DISARMED' (solid green LED).
   2. Idle (DISARMED): Waits for commands or night auto-arm (LDR).
   3. Arming: 'ARMING' (fast green blink) for 3s.
   4. Active (ARMED): 'ARMED' (slow red "heartbeat" pulse).
   5. Intrusion Pending: 'INTRUSION_PENDING' (fast red flash) for 3s.
   6. Alarming: 'ALARMING' (rapid red flash + buzzer) until disarmed.

   ── PIN MAP (UNO) ───────────────────────────────────────────────
   HC-SR04: TRIG → D7,  ECHO → D4
   RED LED: + → D10 (PWM) (via resistor) → GND
   GRN LED: + → D6 (PWM)  (via resistor) → GND
   BUZZER : + → D9,  − → GND
   LDR    : LDR leg1 → 5V, LDR leg2 → A0 and 10kΩ → GND
*/

#include <Arduino.h>

// --- Hardware Test Mode ---
// Set to 0 for normal alarm operation.
// Set to 1-4 to run a specific hardware test instead.
constexpr int RUN_TEST_MODE = 0;
// 1: Test Red/Green LEDs
// 2: Test Buzzer
// 3: Test Ultrasonic Sensor
// 4: Test LDR Sensor
// 5: easiest test on the planet

// --- Pins ---
constexpr uint8_t PIN_TRIG      = 7;
constexpr uint8_t PIN_ECHO      = 4;   // 
constexpr uint8_t PIN_LED_RED   = 10;  // PWM (Timer1)
constexpr uint8_t PIN_LED_GREEN = 6;   // PWM (Timer0)
constexpr uint8_t PIN_BUZZER    = 9;   // tone output
constexpr uint8_t PIN_LDR       = A0;

// --- Runtime Settings ---
struct Settings {
  String   systemName      = "HomeAlarm";
  uint16_t ldrThreshold    = 400;    // ADC (0..1023). Below this value = dark
  float    ultrasonicDeltaCm = 15.0;   // movement tolerance from baseline
  uint16_t buzzerToneHz    = 1200;   // alarm tone
  String   password        = "12";
} cfg;

// --- State Machine & Timers ---
enum class State : uint8_t { DISARMED, ARMING, ARMED, INTRUSION_PENDING, ALARMING };
State state_ = State::DISARMED;

unsigned long nowMs = 0;
unsigned long tStateStart = 0; // When did we enter the current state?
const unsigned long ARMING_DELAY_MS = 3000;
const unsigned long ALARM_DELAY_MS  = 3000;

// Vars for ultrasonic baseline calibration
bool          baselineReady   = false;
float         baselineCm      = 0.0f;
unsigned long tBaselineStart  = 0;
const unsigned long BASELINE_WARMUP_MS = 2000; // Time to ignore readings at boot
const uint8_t       BASELINE_SAMPLES   = 25;   // Samples to average

// Vars for Serial Menu/UI
String lineBuf;
bool   inSettings       = false;
bool   awaitingPassword = false;
String pendingAction;          // What to do *after* password OK? e.g., "disarm", "setU", "change_pwd_*"

// Timers for non-blocking LED/Buzzer effects
unsigned long tBlink  = 0; bool ledBlink = false;
unsigned long tBuzzer = 0; bool buzzerOn = false;


// =================================================================
//                 --- HARDWARE TEST FUNCTIONS ---
// =================================================================

// Helper to set brightness of the two LEDs
static inline void ledWrite(uint8_t r, uint8_t g) {
  analogWrite(PIN_LED_RED, r);
  analogWrite(PIN_LED_GREEN, g);
}

static inline void ledsOff() { ledWrite(0,0); }

// TEST 1: Cycle through Red, Green, Both
void runTest_LEDs() {
  static unsigned long tLastLedChange = 0;
  static int ledTestState = 0; // 0=Red, 1=Green, 2=Both
  const unsigned long LED_TEST_INTERVAL = 1000;

  if (nowMs - tLastLedChange >= LED_TEST_INTERVAL) {
    tLastLedChange = nowMs;
    ledTestState = (ledTestState + 1) % 3;
    
    if (ledTestState == 0) { 
      ledWrite(255, 0); Serial.println("TEST: RED"); 
    } else if (ledTestState == 1) { 
      ledWrite(0, 255); Serial.println("TEST: GREEN"); 
    } else { 
      ledWrite(255, 255); Serial.println("TEST: BOTH"); 
    }
  }
}

// TEST 2: Beep on and off
void runTest_Buzzer() {
  static unsigned long tLastBuzzerChange = 0;
  static bool isBuzzerTestOn = false;
  const unsigned long BUZZER_TEST_INTERVAL = 1500;
  
  // Need to define buzzer() here for the test, or move helpers up
  auto buzzer = [](bool on) {
    if (on) tone(PIN_BUZZER, cfg.buzzerToneHz);
    else    noTone(PIN_BUZZER);
  };

  if (nowMs - tLastBuzzerChange >= BUZZER_TEST_INTERVAL) {
    tLastBuzzerChange = nowMs;
    isBuzzerTestOn = !isBuzzerTestOn;
    buzzer(isBuzzerTestOn);
    Serial.println(isBuzzerTestOn ? "TEST: Buzzer ON" : "TEST: Buzzer OFF");
  }
}

void runTest_Easy(){
    digitalWrite(10, HIGH);  // red ON
  digitalWrite(6,  LOW);   // green OFF
  delay(700);

  digitalWrite(10, LOW);   // red OFF
  digitalWrite(6,  HIGH);  // green ON
  delay(700);

  digitalWrite(10, HIGH);  // both ON
  digitalWrite(6,  HIGH);
  delay(700);

  digitalWrite(10, LOW);   // both OFF
  digitalWrite(6,  LOW);
  delay(700);
}

// Read distance in cm. Returns NAN if pulseIn times out.
float readUltrasonicCm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 30000UL); 
  if (us == 0) return NAN; // Timeout
  return (us * 0.0343f) / 2.0f;
}

// TEST 3: Read and print ultrasonic distance
void runTest_Ultrasonic() {
  static unsigned long tLastUltraRead = 0;
  const unsigned long ULTRA_TEST_INTERVAL = 500; // 0.5 sec

  if (nowMs - tLastUltraRead >= ULTRA_TEST_INTERVAL) {
    tLastUltraRead = nowMs;
    float dist = readUltrasonicCm();
    Serial.print("TEST: Ultrasonic (cm): ");
    if (isnan(dist)) {
      Serial.println("TIMEOUT");
    } else {
      Serial.println(dist, 2);
    }
  }
}

int readLDR() { return analogRead(PIN_LDR); }

// TEST 4: Read and print LDR value
void runTest_LDR() {
  static unsigned long tLastLdrRead = 0;
  const unsigned long LDR_TEST_INTERVAL = 500; // 0.5 sec

  if (nowMs - tLastLdrRead >= LDR_TEST_INTERVAL) {
    tLastLdrRead = nowMs;
    int ldrVal = readLDR();
    Serial.print("TEST: LDR (0-1023): ");
    Serial.println(ldrVal);
  }
}

// --- Helper Functions (for Normal Mode) ---

void buzzer(bool on) {
  if (on) tone(PIN_BUZZER, cfg.buzzerToneHz);
  else    noTone(PIN_BUZZER);
}

// Gathers baseline ultrasonic reading on startup.
void updateBaselineStartup() {
  if (baselineReady) return;
  if (tBaselineStart == 0) tBaselineStart = nowMs;

  static uint8_t n = 0;
  static float acc = 0.0f;

  // Wait for warmup period, then take samples
  if (nowMs - tBaselineStart >= BASELINE_WARMUP_MS && n < BASELINE_SAMPLES) {
    float d = readUltrasonicCm();
    if (!isnan(d)) { acc += d; n++; }
    
    // Got enough samples? Calculate average and flag as ready.
    if (n >= BASELINE_SAMPLES) {
      baselineCm = acc / n;
      baselineReady = true;
      Serial.print("[Init] Ultrasonic baseline (cm): ");
      Serial.println(baselineCm, 1);
      Serial.println("Type '1' to Arm, or wait for dark auto-arm.");
    }
  }
}

// Check if current reading is "different enough" from baseline
bool intrusionDetected() {
  float d = readUltrasonicCm();
  bool moved = (!isnan(d)) && (fabs(d - baselineCm) >= cfg.ultrasonicDeltaCm);
  return moved;
}

// --- State Transition Functions ---
// (These functions run *once* when changing state)

void go(State s) { state_ = s; tStateStart = nowMs; }

void enterDisarmed() {
  go(State::DISARMED);
  buzzer(false);
  ledWrite(0, 180); // solid green
  Serial.println("System DISARMED.");
}

void enterArming() {
  go(State::ARMING);
  Serial.println("Arming... (3s)");
}

void enterArmed() {
  go(State::ARMED);
  ledsOff(); // LED off, will be pulsed by 'heartbeat' in loop()
  Serial.println("System ARMED.");
}

void enterIntrusionPending() {
  go(State::INTRUSION_PENDING);
  Serial.println("Intrusion detected! Alarm in 3s. Enter password to cancel:");
  awaitingPassword = true; pendingAction = "abort_alarm";
}

void enterAlarming() {
  go(State::ALARMING);
  Serial.println("ALARMING! Enter password to disarm:");
}

// --- Password Handling ---

void promptPassword(const String& actionTag) {
  awaitingPassword = true;
  pendingAction = actionTag;
  Serial.print("Enter password: ");
}

// This runs when user hits ENTER and we were waiting for a password
void handlePasswordIfAny(const String& pwd) {
  if (!awaitingPassword) return;
  awaitingPassword = false;

  if (pwd == cfg.password) {
    // Check what we were trying to do
    if (pendingAction == "disarm") {
      Serial.println("OK. Disarming.");
      enterDisarmed();
    } else if (pendingAction == "abort_alarm") {
      Serial.println("OK. Alarm canceled.");
      enterDisarmed();
    } else if (pendingAction == "change_pwd_old_ok") {
      Serial.println("OK. Enter NEW password:");
      awaitingPassword = true; // Ask for *another* line
      pendingAction = "change_pwd_new";
    } else if (pendingAction == "change_pwd_new") {
      cfg.password = pwd;
      Serial.println("Password changed.");
    }
  } else {
    // Wrong password
    Serial.println("Password incorrect!");
    if (state_ == State::ALARMING) Serial.println("Still ALARMING!");
  }
  
  if (pendingAction != "change_pwd_new") {
      pendingAction = ""; // Clear action unless we're mid-change
  }
}

// --- Serial Menu UI ---

void printMainMenu() {
  Serial.println();
  Serial.println(F("=== HOME ALARM MENU ==="));
  Serial.print(F("System: ")); Serial.println(cfg.systemName);
  Serial.print(F("State : "));
  Serial.println(state_ == State::DISARMED ? "DISARMED" :
                 state_ == State::ARMING ? "ARMING" :
                 state_ == State::ARMED ? "ARMED" :
                 state_ == State::INTRUSION_PENDING ? "INTRUSION_PENDING" :
                 "ALARMING");
  if (state_ == State::DISARMED) {
    Serial.println(F("[1] Arm system"));
    Serial.println(F("[2] Test alarm"));
    Serial.println(F("[3] Settings"));
  } else {
    Serial.println(F("[D] Disarm (password)"));
  }
}

void printSettingsMenu() {
  Serial.println();
  Serial.println(F("--- Settings ---"));
  Serial.print(F("Name: ")); Serial.println(cfg.systemName);
  Serial.print(F("Ultrasonic delta (cm): ")); Serial.println(cfg.ultrasonicDeltaCm, 1);
  Serial.print(F("LDR threshold (0-1023): ")); Serial.println(cfg.ldrThreshold);
  Serial.print(F("Buzzer tone (Hz): ")); Serial.println(cfg.buzzerToneHz);
  Serial.println(F("[U] Set ultrasonic delta"));
  Serial.println(F("[L] Set LDR threshold"));
  Serial.println(F("[B] Set buzzer tone (Hz)"));
  Serial.println(F("[N] Set system name"));
  Serial.println(F("[P] Change password"));
  Serial.println(F("[X] Back"));
}

// Main serial command router
void handleLine(String s) {
  s.trim();
  if (!s.length()) return; // Ignore empty lines

  // 1. Are we waiting for a password?
  if (awaitingPassword) { handlePasswordIfAny(s); return; }

  // 2. Are we waiting for a *value* in the settings menu?
  if (inSettings && (pendingAction == "setU" || pendingAction == "setL" || pendingAction == "setB" || pendingAction == "setN")) {
    if (pendingAction == "setU") {
      cfg.ultrasonicDeltaCm = s.toFloat();
      Serial.println("Ultrasonic delta updated.");
    } else if (pendingAction == "setL") {
      cfg.ldrThreshold = constrain(s.toInt(), 0, 1023);
      Serial.println("LDR threshold updated.");
    } else if (pendingAction == "setB") {
      cfg.buzzerToneHz = max(100, s.toInt()); // Keep it audible
      Serial.println("Buzzer tone updated.");
    } else if (pendingAction == "setN") {
      cfg.systemName = s;
      Serial.println("Name updated.");
    }
    pendingAction = ""; // Clear action
    printSettingsMenu();
    return;
  }

  // 3. Are we in the Settings menu (command entry)?
  if (inSettings) {
    if (s.equalsIgnoreCase("X")) { inSettings = false; printMainMenu(); return; }
    if (s.equalsIgnoreCase("U")) { Serial.println("Enter new delta (cm):"); pendingAction = "setU"; return; }
    if (s.equalsIgnoreCase("L")) { Serial.println("Enter new LDR threshold (0-1023):"); pendingAction = "setL"; return; }
    if (s.equalsIgnoreCase("B")) { Serial.println("Enter new buzzer tone (Hz):"); pendingAction = "setB"; return; }
    if (s.equalsIgnoreCase("N")) { Serial.println("Enter new system name:"); pendingAction = "setN"; return; }
    if (s.equalsIgnoreCase("P")) { Serial.println("Enter CURRENT password:"); awaitingPassword = true; pendingAction = "change_pwd_old_ok"; return; }

    Serial.println("Unknown settings command.");
    printSettingsMenu();
    return;
  }

  // 4. Must be in the Main menu
  if (state_ == State::DISARMED) {
    if (s == "1")      { enterArming(); }
    else if (s == "2") { enterAlarming(); }
    else if (s == "3") { inSettings = true; printSettingsMenu(); }
    else               { printMainMenu(); }
  } else {
    // System is ARMED, ARMING, etc.
    if (s.equalsIgnoreCase("D")) { promptPassword("disarm"); }
    else { printMainMenu(); }
  }
}

// --- Main Setup & Loop ---

void setup() {
  Serial.begin(115200);

  // Pin modes are required for all modes (test or normal)
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  // PIN_LDR (A0) is analog input, no pinMode needed

  // Check if we are in a test mode
  if (RUN_TEST_MODE > 0) {
    Serial.println(F("============================="));
    Serial.print(F("!!! HARDWARE TEST MODE "));
    Serial.print(RUN_TEST_MODE);
    Serial.println(F(" ON !!!"));
    Serial.println(F("============================="));
    Serial.println(F("Reset board to return to normal."));
    
    // Ensure outputs are off at start of test
    ledsOff();
    buzzer(false);
  } else {
    // Normal operation
    enterDisarmed();         // Start DISARMED (solid green)
    Serial.println("Booting...");
    Serial.print("System: "); Serial.println(cfg.systemName);
    printMainMenu();
  }
}

void loop() {
  nowMs = millis();

  // Route to TEST function or NORMAL operation
  switch (RUN_TEST_MODE) {
    case 1:
      runTest_LEDs();
      break;
    case 2:
      runTest_Buzzer();
      break;
    case 3:
      runTest_Ultrasonic();
      break;
    case 4:
      runTest_LDR();
      break;
    case 5:
      runTest_Easy();
      break;
      
    case 0: // Normal operation
    default: 
    {
      // --- ALL NORMAL ALARM LOGIC IS BELOW ---
      
      // Keep updating baseline until it's ready
      updateBaselineStartup();

      // Process any incoming serial data (non-blocking)
      while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue; // Ignore carriage return
        if (c == '\n') {
          String s = lineBuf;
          lineBuf = ""; // Clear buffer
          handleLine(s);
        }
        else {
          lineBuf += c;
        }
      }

      // Auto-arm if it gets dark (LDR) and system is disarmed
      if (state_ == State::DISARMED && baselineReady) {
        if (readLDR() < cfg.ldrThreshold) {
          Serial.println("Night auto-arm...");
          enterArming();
        }
      }

      // === Main State Machine ===
      // (This runs *every loop* to update effects/check sensors)
      switch (state_) {
        case State::DISARMED:
          // Solid green (set in enterDisarmed())
          break;

        case State::ARMING: {
          // Fast green blink
          if (nowMs - tBlink >= 250) { tBlink = nowMs; ledBlink = !ledBlink; }
          ledWrite(0, ledBlink ? 200 : 0);
          // Check if timer is up
          if (nowMs - tStateStart >= ARMING_DELAY_MS) enterArmed();
        } break;

        case State::ARMED: {
          // Slow red "heartbeat" pulse
          if (nowMs - tBlink >= 1500) { tBlink = nowMs; ledBlink = !ledBlink; }
          ledWrite(ledBlink ? 50 : 0, 0);
          // Check sensor
          if (intrusionDetected()) enterIntrusionPending();
        } break;

        case State::INTRUSION_PENDING: {
          // Fast red flash (giving user time to cancel)
          if (nowMs - tBlink >= 250) { tBlink = nowMs; ledBlink = !ledBlink; }
          ledWrite(ledBlink ? 220 : 0, 0);
          // Check if time is up
          if (nowMs - tStateStart >= ALARM_DELAY_MS) enterAlarming();
        } break;

        case State::ALARMING: {
          // Rapid flash and buzzer chirp
          if (nowMs - tBuzzer >= 200) { tBuzzer = nowMs; buzzerOn = !buzzerOn; buzzer(buzzerOn); }
          if (nowMs - tBlink  >= 150) { tBlink  = nowMs; ledBlink = !ledBlink; }
          ledWrite(ledBlink ? 255 : 0, 0);
        } break;
      } // end switch(state_)
    } // end case 0
    break; 
  } // end switch(RUN_TEST_MODE)
}