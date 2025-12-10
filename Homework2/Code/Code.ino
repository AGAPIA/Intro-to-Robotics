/* ---------------------------------------------------------
 * Homework #2 – Traffic Lights (Common-Cathode 7-seg)
 * --------------------------------------------------------- */

#include <Arduino.h>

/* ---------- Pins ---------- */
// Button + buzzer
const uint8_t PIN_BTN  = 2;      // momentary push-button to GND (INPUT_PULLUP)
const uint8_t PIN_BUZZ = 11;     // piezo buzzer (ON with HIGH)

// Car LEDs (individual R/Y/G)
const uint8_t PIN_CAR_G = A0;
const uint8_t PIN_CAR_Y = A1;
const uint8_t PIN_CAR_R = A2;

// Pedestrian LEDs (red, green)
const uint8_t PIN_PED_R = A3;
const uint8_t PIN_PED_G = A4;

// 7-segment segments A..G and DP (one resistor per segment)
const uint8_t SEG_A  = 12;
const uint8_t SEG_B  = 10;
const uint8_t SEG_C  = 9;
const uint8_t SEG_D  = 8;
const uint8_t SEG_E  = 7;
const uint8_t SEG_F  = 6;
const uint8_t SEG_G  = 5;
const uint8_t SEG_DP = 4;      // optional decimal point

// Common-cathode display => segments are ACTIVE HIGH
const bool SEG_ACTIVE_HIGH = true;
const bool USE_DP          = false;

/* ---------- Timings (ms) ---------- */
const uint32_t DLY_AFTER_PRESS = 8000;  // S1: wait after press (cars green)
const uint32_t DUR_S2          = 3000;  // S2: cars yellow
const uint32_t DUR_S3          = 8000;  // S3: peds green (slow beep)
const uint32_t DUR_S4          = 4000;  // S4: peds blinking green (fast beep)

// Beep/blink cadences
const uint16_t BEEP_S3_MS  = 500;
const uint16_t BEEP_S4_MS  = 200;
const uint16_t BLINK_S4_MS = 250;

/* ---------- State machine ---------- */
enum class State : uint8_t { Idle, WaitingDelay, CarsYellow, PedsGreen, PedsBlink };

volatile bool     g_btnEdge   = false;  // set by ISR
volatile uint32_t g_lastIsrMs = 0;      // debounce

State     g_state        = State::Idle;
uint32_t  g_tStateStart  = 0;
uint32_t  g_tNextBeep    = 0;
bool      g_beepOn       = false;
uint32_t  g_tNextBlink   = 0;
bool      g_blinkOn      = true;
uint32_t  g_waitStart    = 0;

/* ---------- 7-segment digits (CC: 1 = ON) ---------- */
const uint8_t SEG_PINS[7] = { SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G };

//            a b c d e f g
const uint8_t DIGIT_PAT[10][7] = {
  {1,1,1,1,1,1,0},  // 0
  {0,1,1,0,0,0,0},  // 1
  {1,1,0,1,1,0,1},  // 2
  {1,1,1,1,0,0,1},  // 3
  {0,1,1,0,0,1,1},  // 4
  {1,0,1,1,0,1,1},  // 5
  {1,0,1,1,1,1,1},  // 6
  {1,1,1,0,0,0,0},  // 7
  {1,1,1,1,1,1,1},  // 8
  {1,1,1,1,0,1,1}   // 9
};

/* ---------- Utilities ---------- */
inline void segWrite(uint8_t pin, bool on) {
  digitalWrite(pin, (SEG_ACTIVE_HIGH ? on : !on) ? HIGH : LOW);
}

void segOff() {
  for (uint8_t i = 0; i < 7; ++i) segWrite(SEG_PINS[i], false);
  if (USE_DP) segWrite(SEG_DP, false);
}

void segShowDigit(int d) {
  if (d < 0 || d > 9) { segOff(); return; }
  for (uint8_t i = 0; i < 7; ++i) segWrite(SEG_PINS[i], DIGIT_PAT[d][i]);
}

void buzzer(bool on) { digitalWrite(PIN_BUZZ, on ? HIGH : LOW); }

void setCar(bool r, bool y, bool g) {
  digitalWrite(PIN_CAR_R, r ? HIGH : LOW);
  digitalWrite(PIN_CAR_Y, y ? HIGH : LOW);
  digitalWrite(PIN_CAR_G, g ? HIGH : LOW);
}

void setPed(bool r, bool g) {
  digitalWrite(PIN_PED_R, r ? HIGH : LOW);
  digitalWrite(PIN_PED_G, g ? HIGH : LOW);
}

int secondsRemaining(uint32_t durMs) {
  uint32_t now = millis(), end = g_tStateStart + durMs;
  if (now >= end) return 0;
  // round up to nearest second
  return (int)((end - now + 999) / 1000);
}

/* ---------- Button ISR (debounced) ---------- */
void onBtn() {
  uint32_t now = millis();
  if (now - g_lastIsrMs < 50) return;  // 50 ms debounce
  g_lastIsrMs = now;
  g_btnEdge   = true;
}

/* ---------- State entry ---------- */
void enter(State s) {
  g_state       = s;
  g_tStateStart = millis();
  g_tNextBeep   = g_tStateStart;
  g_tNextBlink  = g_tStateStart;
  g_beepOn      = false;
  g_blinkOn     = true;

  switch (g_state) {
    case State::Idle:
    case State::WaitingDelay:
      segOff(); buzzer(false);
      setCar(false, false, true);  // cars green
      setPed(true,  false);        // peds red
      break;

    case State::CarsYellow:
      segOff(); buzzer(false);
      setCar(false, true,  false); // cars yellow
      setPed(true,  false);
      break;

    case State::PedsGreen:
      setCar(true,  false, false); // cars red
      setPed(false, true);         // peds green
      break;

    case State::PedsBlink:
      setCar(true,  false, false); // cars stay red
      // ped green will blink in loop()
      break;
  }
}

/* ---------- Setup/Loop ---------- */
void setup() {
  pinMode(PIN_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN), onBtn, FALLING);

  pinMode(PIN_BUZZ, OUTPUT);
  pinMode(PIN_CAR_R, OUTPUT);
  pinMode(PIN_CAR_Y, OUTPUT);
  pinMode(PIN_CAR_G, OUTPUT);
  pinMode(PIN_PED_R, OUTPUT);
  pinMode(PIN_PED_G, OUTPUT);

  for (uint8_t i = 0; i < 7; ++i) pinMode(SEG_PINS[i], OUTPUT);
  if (USE_DP) pinMode(SEG_DP, OUTPUT);

  // deterministic OFF/idle
  segOff();
  buzzer(false);
  setCar(false, false, false);
  setPed(false, false);

  enter(State::Idle);
}

void loop() {
  uint32_t now = millis();

  // One-shot: start the sequence from Idle
  if (g_btnEdge) {
    noInterrupts();
    bool pressed = g_btnEdge;
    g_btnEdge = false;
    interrupts();
    if (pressed && g_state == State::Idle) {
      g_waitStart = now;
      enter(State::WaitingDelay);
    }
  }

  // The S1 waiting delay happens while cars remain green
  if (g_state == State::WaitingDelay) {
    if (now - g_waitStart >= DLY_AFTER_PRESS) enter(State::CarsYellow);
    return;
  }

  // Main state advances
  switch (g_state) {
    case State::Idle:
      // steady cars-G / peds-R; wait for button
      break;

    case State::CarsYellow: {
      segShowDigit(secondsRemaining(DUR_S2));
      if (now - g_tStateStart >= DUR_S2)
        enter(State::PedsGreen);
    } break;

    case State::PedsGreen: {
      segShowDigit(secondsRemaining(DUR_S3));
      // slow beep
      if (now >= g_tNextBeep) {
        g_beepOn = !g_beepOn;
        buzzer(g_beepOn);
        g_tNextBeep = now + BEEP_S3_MS;
      }
      if (now - g_tStateStart >= DUR_S3) {
        buzzer(false);
        enter(State::PedsBlink);
      }
    } break;

    case State::PedsBlink: {
      segShowDigit(secondsRemaining(DUR_S4));
      // blink ped green
      if (now >= g_tNextBlink) {
        g_blinkOn   = !g_blinkOn;
        g_tNextBlink = now + BLINK_S4_MS;
        setPed(true, g_blinkOn);   // ped red ON, green blinking
      }
      // faster beep
      if (now >= g_tNextBeep) {
        g_beepOn = !g_beepOn;
        buzzer(g_beepOn);
        g_tNextBeep = now + BEEP_S4_MS;
      }
      if (now - g_tStateStart >= DUR_S4) {
        buzzer(false);
        enter(State::Idle);
      }
    } break;

    case State::WaitingDelay: break; // handled above
  }
}
