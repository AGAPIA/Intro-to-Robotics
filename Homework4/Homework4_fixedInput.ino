#include <Arduino.h>
#include <SPI.h>
// =================== GAME STATE ===================

enum State {
  STATE_IDLE_MENU,
  STATE_SHOW_SEQ,
  STATE_INPUT,
  STATE_CHECK,
  STATE_RESULT,
  STATE_PAUSED,
};

// =================== DEBUGGING SUPPORT ======
// If 1 => use fixed sequence for easier testing.
// If 0 => use random sequence.
#define USE_FIXED_SEQUENCE 1
const char FIXED_SEQ[4] = {'1', '2', '3', '4'}; // chars must be in the charset

// =================== PINS ===================

// 74HC595
// const uint8_t PIN_LATCH = 11;   // STCP
// const uint8_t PIN_CLOCK = 10;   // SHCP
// const uint8_t PIN_DATA  = 12;   // DS

const uint8_t PIN_LATCH = 10;   // now CS/SS pin
const uint8_t PIN_CLOCK = 13;   // now SCK pin
const uint8_t PIN_DATA  = 11;   // now MOSI pin

// 4-digit 7-seg CC digits (common cathode, active LOW)
const uint8_t PIN_DIG0  = 4;    // rightmost (D1)
const uint8_t PIN_DIG1  = 5;    // D2
const uint8_t PIN_DIG2  = 6;    // D3
const uint8_t PIN_DIG3  = 7;    // leftmost (D4)
const uint8_t DIG_PINS[4] = { PIN_DIG0, PIN_DIG1, PIN_DIG2, PIN_DIG3 };

// Joystick
const uint8_t PIN_JOY_X  = A1;
const uint8_t PIN_JOY_Y  = A2;
const uint8_t PIN_JOY_SW = 8;   // SW (active LOW, INPUT_PULLUP)

// Play/Stop button
const uint8_t PIN_BTN_PLAY = 3; // active LOW, INPUT_PULLUP

// =================== BUTTON HELPER ===================

struct Btn {
  uint8_t pin;
  bool    last;
  unsigned long tLast;

  Btn(uint8_t p) {
    pin  = p;
    last = HIGH;
    tLast = 0;
  }

  bool read() {  // debounced
    bool v = digitalRead(pin);
    unsigned long now = millis();
    const unsigned long DEBOUNCE_MS = 40;
    if (v != last && (now - tLast) < DEBOUNCE_MS) {
      return last;
    }
    if (v != last) {
      last = v;
      tLast = now;
    }
    return v;
  }
};

Btn btnSW(PIN_JOY_SW);
Btn btnPLAY(PIN_BTN_PLAY);

// =================== SEGMENT ENCODING ===================
// Q7->A, Q6->B, Q5->C, Q4->D, Q3->E, Q2->F, Q1->G, Q0->DP

const uint8_t SEG_A  = 0b10000000;
const uint8_t SEG_B  = 0b01000000;
const uint8_t SEG_C  = 0b00100000;
const uint8_t SEG_D  = 0b00010000;
const uint8_t SEG_E  = 0b00001000;
const uint8_t SEG_F  = 0b00000100;
const uint8_t SEG_G  = 0b00000010;
const uint8_t SEG_DP = 0b00000001;

const uint8_t F0 = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
const uint8_t F1 = SEG_B | SEG_C;
const uint8_t F2 = SEG_A | SEG_B | SEG_D | SEG_E | SEG_G;
const uint8_t F3 = SEG_A | SEG_B | SEG_C | SEG_D | SEG_G;
const uint8_t F4 = SEG_F | SEG_G | SEG_B | SEG_C;
const uint8_t F5 = SEG_A | SEG_F | SEG_G | SEG_C | SEG_D;
const uint8_t F6 = SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D;
const uint8_t F7 = SEG_A | SEG_B | SEG_C;
const uint8_t F8 = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t F9 = SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;

const uint8_t FA = SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
const uint8_t FP = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;
const uint8_t FL = SEG_D | SEG_E | SEG_F;
const uint8_t FS = F5;
const uint8_t FT = SEG_A | SEG_E | SEG_F ;
const uint8_t FO = F0;
const uint8_t FR = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G | SEG_C;
const uint8_t FC = SEG_A | SEG_F | SEG_E | SEG_D;
const uint8_t FE = SEG_A | SEG_F | SEG_E | SEG_D | SEG_G;
const uint8_t Fn = SEG_C | SEG_E | SEG_G;
const uint8_t FU = SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
const uint8_t FY = SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;
const uint8_t Fh = SEG_C | SEG_E | SEG_F | SEG_G;

// NEW: G and D so "GOOD" renders properly
const uint8_t FG = SEG_A | SEG_F | SEG_E | SEG_C | SEG_D;   // G
const uint8_t FD = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;   // D-like

uint8_t mapChar(char ch) {
  switch (ch) {
    case '0': return F0;
    case '1': return F1;
    case '2': return F2;
    case '3': return F3;
    case '4': return F4;
    case '5': return F5;
    case '6': return F6;
    case '7': return F7;
    case '8': return F8;
    case '9': return F9;

    case 'A': return FA;
    case 'C': return FC;
    case 'D': return FD;   // NEW
    case 'E': return FE;
    case 'G': return FG;   // NEW
    case 'L': return FL;
    case 'O': return FO;
    case 'P': return FP;
    case 'R': return FR;
    case 'S': return FS;
    case 'T': return FT;
    case 'U': return FU;
    case 'Y': return FY;

    case 't': return FT;
    case 'y': return FY;
    case 'r': return FR;
    case 'n': return Fn;
    case 'h': return Fh;

    case '-': return SEG_G;
    case ' ': return 0;
    default:  return 0;
  }
}

// =================== SHIFT REG + DISPLAY ===================

volatile uint8_t frame[4] = {0, 0, 0, 0};
unsigned long lastMux = 0;
uint8_t muxIndex = 0;
const unsigned int DIGIT_ON_US = 1200;

void writeReg(uint8_t value) {
  digitalWrite(PIN_LATCH, LOW);
  //shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, value);
  SPI.transfer(value);
  digitalWrite(PIN_LATCH, HIGH);
}

void allDigitsOff() {
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(DIG_PINS[i], HIGH); // CC: HIGH=off
  }
}

void enableDigit(uint8_t idx) {
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(DIG_PINS[i], HIGH);
  }
  digitalWrite(DIG_PINS[idx], LOW);  // CC: LOW=on
}

void displayService() {
  unsigned long now = micros();
  if (now - lastMux < DIGIT_ON_US) return;
  lastMux = now;

  muxIndex = (muxIndex + 1) & 0x03;
  allDigitsOff();
  writeReg(frame[muxIndex]);
  enableDigit(muxIndex);
}

void print4(const char s[4]) {
  for (uint8_t i = 0; i < 4; i++) {
    frame[i] = mapChar(s[i]);
  }
  Serial.print(F("[DISPLAY] '"));
  Serial.write(s, 4);
  Serial.println("'");
}

// =================== GAME DATA ===================

State state_ = STATE_IDLE_MENU;

struct Menu {
  const char* items[3];
  int idx;
};

Menu menu = { {"PLAY", "SCOR", "STOP"}, 0 };

uint8_t seq[4];
uint8_t user[4];
bool    locked[4] = {false,false,false,false};
uint8_t cursor = 0;

unsigned long tState      = 0;
unsigned long tSeqStart   = 0;
unsigned long tBlinkFast  = 0;
bool blinkFast = false;

State prevStateForPause;
unsigned long tPauseStart = 0;

// Difficulty / timings
const uint16_t SHOW_TIME_MAX_VALUE    = 2000; // Start level
const uint16_t SHOW_TIME_MIN_VALUE    =  800; // Hardest
const uint16_t SHOW_TIME_MS_PER_LEVEL =  200; // step

uint16_t showTimeMs = SHOW_TIME_MAX_VALUE;
uint16_t score      = 0;
uint16_t highScore  = 0;

const char GAMESET[] = {'0','1','2','3','4','5','6','7','8','9','A','C','E','L','P','S','U','Y'};
const int  GAMESET_N = sizeof(GAMESET);

// Return the index of a char in GAMESET
int findGameCharIndex(char ch) {
  for (int i = 0; i < GAMESET_N; i++) {
    if (GAMESET[i] == ch) return i;
  }
  return 0; // fallback to GAMESET[0] if not found
}

// Threshold to diff between short and long press
uint16_t shortPressThreshold = 1000;

// =================== JOYSTICK HELPERS ===================

const int JOY_DEAD = 260;

int joyX() { return (int)analogRead(PIN_JOY_X) - 512; }
int joyY() { return (int)analogRead(PIN_JOY_Y) - 512; }

bool joyLeftNow()  { return joyX() < -JOY_DEAD; }
bool joyRightNow() { return joyX() >  JOY_DEAD; }
bool joyUpNow()    { return joyY() >  JOY_DEAD; }
bool joyDownNow()  { return joyY() < -JOY_DEAD; }

// Edge detectors with init guards

bool joyLeftEdge() {
  static bool prev = false, init = false;
  bool cur = joyLeftNow();
  if (!init) { prev = cur; init = true; return false; }
  bool fire = (cur && !prev);
  prev = cur;
  if (fire) Serial.println(F("[JOY] Left edge"));
  return fire;
}

bool joyRightEdge() {
  static bool prev = false, init = false;
  bool cur = joyRightNow();
  if (!init) { prev = cur; init = true; return false; }
  bool fire = (cur && !prev);
  prev = cur;
  if (fire) Serial.println(F("[JOY] Right edge"));
  return fire;
}

bool joyUpEdge() {
  static bool prev = false, init = false;
  bool cur = joyUpNow();
  if (!init) { prev = cur; init = true; return false; }
  bool fire = (cur && !prev);
  prev = cur;
  if (fire) Serial.println(F("[JOY] Up edge"));
  return fire;
}

bool joyDownEdge() {
  static bool prev = false, init = false;
  bool cur = joyDownNow();
  if (!init) { prev = cur; init = true; return false; }
  bool fire = (cur && !prev);
  prev = cur;
  if (fire) Serial.println(F("[JOY] Down edge"));
  return fire;
}

void enterPaused() {
  prevStateForPause = state_;
  tPauseStart = millis();
  setState(STATE_PAUSED);
  print4("PAUS");
}

void resumeFromPause() {
  unsigned long d = millis() - tPauseStart;
  // keep timers consistent after pause
  tState    += d;
  tSeqStart += d;
  tBlinkFast += d;
  setState(prevStateForPause);
}

// =================== INPUT EVENT GATING ===================

// Event codes for joystick switch
#define EV_NONE  0u
#define EV_SHORT 1u
#define EV_LONG  2u

// Ignore inputs for a small time after entering a state
inline bool stateSettled(uint16_t ms = 250) {
  return (millis() - tState) >= ms;
}

// Unified JOY SW event (returns EV_SHORT / EV_LONG once per press)
uint8_t swEvent() {
  static bool was = HIGH;
  static unsigned long tDown = 0;
  static bool longFired = false;
  static State pressStartState = STATE_IDLE_MENU;

  bool v = btnSW.read(); // debounced

  if (was == HIGH && v == LOW) {
    was = LOW;
    longFired = false;
    tDown = millis();
    pressStartState = state_;
    return EV_NONE;
  }

  if (was == LOW) {
    if (v == HIGH) {
      was = HIGH;
      unsigned long d = millis() - tDown;
      if (!longFired && d < shortPressThreshold &&
          pressStartState == state_ && stateSettled()) {
        Serial.println(F("[JOY SW] SHORT"));
        return EV_SHORT;
      }
    } else {
      if (!longFired &&
          (millis() - tDown) >= shortPressThreshold &&
          pressStartState == state_ && stateSettled()) {
        longFired = true;
        Serial.println(F("[JOY SW] LONG"));
        return EV_LONG;
      }
    }
  }

  return EV_NONE;
}

// Play/Stop button long press: only valid if press STARTED in-game
bool btnPlayLongPress() {
  static bool was = HIGH;
  static unsigned long tDown = 0;
  static bool fired = false;
  static State pressStartState = STATE_IDLE_MENU;

  bool v = btnPLAY.read();

  if (was == HIGH && v == LOW) {
    was = LOW;
    fired = false;
    tDown = millis();
    pressStartState = state_;
  } else if (was == LOW) {
    if (v == HIGH) {
      was = HIGH;
    } else {
      if (!fired &&
          (millis() - tDown) >= 700 &&
          pressStartState != STATE_IDLE_MENU &&
          stateSettled()) {
        fired = true;
        Serial.println(F("[BTN] LONG press (game STOP)"));
        return true;
      }
    }
  }
  return false;
}

// Play/Stop button short press (used only in menu)
bool btnPlayShortPress() {
  static bool was = HIGH;
  static unsigned long tDown = 0;
  bool v = btnPLAY.read();
  if (was == HIGH && v == LOW) {
    was = LOW;
    tDown = millis();
  } else if (was == LOW && v == HIGH) {
    was = HIGH;
    unsigned long d = millis() - tDown;
    if (d < shortPressThreshold) {
      Serial.println(F("[BTN] Short press"));
      return true;
    }
  }
  return false;
}

// =================== STATE HELPERS ===================

void logState(const __FlashStringHelper* name) {
  Serial.print(F("[STATE] -> "));
  Serial.println(name);
}

void setState(State s) {
  state_ = s;
  tState = millis();
  switch (s) {
    case STATE_IDLE_MENU: logState(F("IDLE_MENU")); break;
    case STATE_SHOW_SEQ:  logState(F("SHOW_SEQ"));  break;
    case STATE_INPUT:     logState(F("INPUT"));     break;
    case STATE_CHECK:     logState(F("CHECK"));     break;
    case STATE_RESULT:    logState(F("RESULT"));    break;
  }
}

uint8_t randIdx() { return (uint8_t)(random(GAMESET_N)); }

void toIdle() {
  menu.idx = 0; // always return to PLAY
  setState(STATE_IDLE_MENU);
  print4(menu.items[menu.idx]);
  for (int i = 0; i < 4; i++) locked[i] = false;
  Serial.println(F("[GAME] Back to menu (PLAY)"));
}

void startGame() {
#if USE_FIXED_SEQUENCE
  Serial.println(F("[GAME] Using FIXED_SEQ for testing"));
  for (int i = 0; i < 4; i++) {
    seq[i] = findGameCharIndex(FIXED_SEQ[i]);
    Serial.print(F("  seq[")); Serial.print(i); Serial.print(F("] = "));
    Serial.println(GAMESET[seq[i]]);
  }
#else
  for (int i = 0; i < 4; i++) {
    seq[i] = randIdx();
    Serial.print(F("  seq[")); Serial.print(i); Serial.print(F("] = "));
    Serial.println(GAMESET[seq[i]]);
  }
#endif

  score = 0;
  showTimeMs = SHOW_TIME_MAX_VALUE;
  tSeqStart = millis();
  setState(STATE_SHOW_SEQ);
}

void showSeqFrame() {
  char s[4];
  for (int i = 0; i < 4; i++) s[i] = GAMESET[seq[i]];
  print4(s);
}

void initInput() {
  Serial.println(F("[INPUT] initInput()"));
  for (int i = 0; i < 4; i++) {
    user[i] = 0;
    locked[i] = false;
  }
  cursor = 0;
  setState(STATE_INPUT);
}

void renderInput() {
  char s[4];
  unsigned long now = millis();

  if (now - tBlinkFast >= 150) {
    tBlinkFast = now;
    blinkFast = !blinkFast;
  }

  for (int i = 0; i < 4; i++) {
    s[i] = GAMESET[user[i]];
  }

  if (blinkFast) {
    s[cursor] = ' ';
  }

  print4(s);
}

void checkAnswer() {
  Serial.println(F("[CHECK] Comparing user vs sequence"));
  bool ok = true;
  for (int i = 0; i < 4; i++) {
    char u = GAMESET[user[i]];
    char r = GAMESET[seq[i]];
    Serial.print(F("  pos ")); Serial.print(i);
    Serial.print(F(": user=")); Serial.print(u);
    Serial.print(F(" seq="));  Serial.println(r);
    if (u != r) ok = false;
  }
  Serial.println(ok ? F("[CHECK] OK") : F("[CHECK] FAIL"));
  setState(STATE_RESULT);
}

void renderResult() {
  static bool init = false;
  static bool success = false;
  static unsigned long t0 = 0;

  if (!init) {
    init = true;
    t0 = millis();
    success = true;
    for (int i = 0; i < 4; i++) {
      if (GAMESET[user[i]] != GAMESET[seq[i]]) {
        success = false;
        break;
      }
    }
    Serial.print(F("[RESULT] "));
    Serial.println(success ? F("SUCCESS") : F("FAIL"));
  }

  if (success) {
    if (millis() - t0 < 1200) {
      print4("GOOD");
    } else {
      // score & high score
      score++;
      if (score > highScore) {
        highScore = score;
        Serial.print(F("[RESULT] New highScore = "));
        Serial.println(highScore);
      }
      // harder next time
      if (showTimeMs > SHOW_TIME_MIN_VALUE) {
        showTimeMs -= SHOW_TIME_MS_PER_LEVEL;
      }
      // next sequence (random for subsequent rounds)
      for (int i = 0; i < 4; i++) seq[i] = randIdx();

      tSeqStart = millis();
      setState(STATE_SHOW_SEQ);
      init = false;
    }
  } else {
    if (millis() - t0 < 1200) {
      print4("Err ");
    } else {
      toIdle();
      init = false;
    }
  }
}

// =================== SETUP ===================

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n[BOOT] Simon Says HW4 starting..."));

  SPI.begin();                 
  //Start the SPI library 
  SPI.setBitOrder(MSBFIRST);  //Set MSB as bit order, this means we will send the most significant bit first     


  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  for (uint8_t i = 0; i < 4; i++) {
    pinMode(DIG_PINS[i], OUTPUT);
    digitalWrite(DIG_PINS[i], HIGH); // CC off
  }

  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY, INPUT_PULLUP);

  randomSeed(analogRead(A0));

  toIdle();
}

// =================== LOOP ===================

void loop() {
  unsigned long now = millis();

  displayService();

  // Global: Play button long-press during game = abort toIdle (only if started in game)
  if (state_ != STATE_IDLE_MENU) {
    // long-press still aborts to menu
    if (btnPlayLongPress()) { Serial.println(F("[BTN] STOP -> menu")); toIdle(); return; }

    // short-press toggles pause/resume
    if (btnPlayShortPress()) {
      if (state_ == STATE_PAUSED) {
        Serial.println(F("[BTN] Resume"));
        resumeFromPause();
      } else if (state_ == STATE_SHOW_SEQ || state_ == STATE_INPUT || state_ == STATE_CHECK) {
        Serial.println(F("[BTN] Pause"));
        enterPaused();
      }
    }
  }
  switch (state_) {
    case STATE_IDLE_MENU: {
      if (joyLeftEdge()) {
        menu.idx = (menu.idx + 2) % 3;
        Serial.print(F("[MENU] idx=")); Serial.println(menu.idx);
        print4(menu.items[menu.idx]);
      }
      if (joyRightEdge()) {
        menu.idx = (menu.idx + 1) % 3;
        Serial.print(F("[MENU] idx=")); Serial.println(menu.idx);
        print4(menu.items[menu.idx]);
      }

      if (stateSettled()) {
        uint8_t ev = swEvent();
        if (ev == EV_SHORT) {
          Serial.println(F("[MENU] Select via JOY SW"));
          const char* item = menu.items[menu.idx];
          if (item[0] == 'P') {
            startGame();
          } else if (item[0] == 'S' && item[1] == 'C') {
            Serial.println(F("[MENU] Show high score"));
            char s[4] = {' ',' ',' ',' '};
            uint16_t v = highScore;
            for (int i = 3; i >= 0; i--) { s[i] = '0' + (v % 10); v /= 10; }
            print4(s);
          } else { // STOP
            Serial.println(F("[MENU] STOP selected (no-op)"));
            print4("STOP");
          }
        }
      }

      if (btnPlayShortPress()) {
        Serial.println(F("[MENU] Select via BTN"));
        const char* item = menu.items[menu.idx];
        if (item[0] == 'P') {
          startGame();
        } else if (item[0] == 'S' && item[1] == 'C') {
          Serial.println(F("[MENU] Show high score"));
          char s[4] = {' ',' ',' ',' '};
          uint16_t v = highScore;
          for (int i = 3; i >= 0; i--) { s[i] = '0' + (v % 10); v /= 10; }
          print4(s);
        } else {
          Serial.println(F("[MENU] STOP selected (no-op)"));
          print4("STOP");
        }
      }
    } break;

    case STATE_SHOW_SEQ:
      showSeqFrame();
      if (now - tSeqStart >= showTimeMs) {
        Serial.println(F("[SHOW_SEQ] Time elapsed -> INPUT"));
        initInput();
      }
      break;

    case STATE_INPUT: {
      renderInput();

      if (joyLeftEdge()) {
        cursor = (cursor + 3) & 0x03;
        Serial.print(F("[INPUT] Cursor -> ")); Serial.println(cursor);
      }
      if (joyRightEdge()) {
        cursor = (cursor + 1) & 0x03;
        Serial.print(F("[INPUT] Cursor -> ")); Serial.println(cursor);
      }

      if (stateSettled()) {
        uint8_t ev = swEvent();
        if (ev == EV_SHORT) {
          locked[cursor] = !locked[cursor];
          Serial.print(F("[INPUT] Digit ")); Serial.print(cursor);
          Serial.println(locked[cursor] ? F(" LOCKED") : F(" UNLOCKED"));
        } else if (ev == EV_LONG) {
          Serial.println(F("[INPUT] Submit via JOY SW LONG"));
          setState(STATE_CHECK);
        }
      }

      if (!locked[cursor]) {
        if (joyUpEdge()) {
          user[cursor] = (user[cursor] + 1) % GAMESET_N;
          Serial.print(F("[INPUT] user[")); Serial.print(cursor);
          Serial.print(F("] = ")); Serial.println(GAMESET[user[cursor]]);
        }
        if (joyDownEdge()) {
          user[cursor] = (user[cursor] + GAMESET_N - 1) % GAMESET_N;
          Serial.print(F("[INPUT] user[")); Serial.print(cursor);
          Serial.print(F("] = ")); Serial.println(GAMESET[user[cursor]]);
        }
      }
    } break;

    case STATE_CHECK:
      checkAnswer();
      break;

    case STATE_RESULT:
      renderResult();
      break;

    case STATE_PAUSED:
      // keep "PAUS" on screen, no timers advance since we compensate on resume.
      break;
  }
}
