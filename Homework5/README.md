# 16x2 LCD Race Game

Small lane-switching race game running on a **16x2 HD44780 LCD** with Arduino, plus a PC console version that shares the same core logic.

---

## 1. Game Overview

- Two lanes: **top row** and **bottom row** of the LCD.
- Car is always at **column 3** and moves **up/down** with the joystick.
- World scrolls from **right to left**:
  - `X` = obstacle (lose 1 life).
  - ♥ (custom glyph) = extra life (max 9).
  - Coin glyph = score bonus.
  - `.` = empty road.
- HUD (top row):
  - `col 0–1`: lives, e.g. `3L`.
  - `col 12–15`: score `000–999`.
- Scoring:
  - +1 point every ~10s survived.
  - +2 points for each coin collected.
- Lives & sounds:
  - Start with **3 lives**.
  - Hit obstacle → lose life + short beep.
  - Lives reach 0 → **Game Over** + longer beep.
- Menu & pause:
  - Main menu: `Start` / `Score` (highscores).
  - Highscores: top 3 scores stored in EEPROM.
  - Long press in-game → **Pause** (`Resume` / `Menu`).

---

## 2. Code Structure

### Core (shared PC & Arduino)

- **`GameCore.h`**
  - `RaceGame` class: keeps 2×24 world, car position, lives, score.
  - Handles scrolling, random spawning, collisions, HUD generation.
- **`GameController.h`**
  - `GameController`: states `MENU`, `PLAYING`, `HISCORES`, `GAMEOVER`.
  - Manages highscores via `IHighscoreStore`.
  - Exposes `getDisplayBuffers(world, hud)` for renderers.

### Arduino-specific

- **`race_game.ino`**
  - `setup()`/`loop()`, ties everything together.
  - Calls input, updates `GameController`, renders via LCD, drives buzzer.
- **`EepromHighscoreStore.h`**
  - Implements `IHighscoreStore` using EEPROM (top 3 scores).
- **`LCDRenderer.h`**
  - Wraps `LiquidCrystal`.
  - Sets custom glyphs: car, heart, coin.
  - Renders menu, game, pause, highscores, game-over screens.
- **`InputService.h`**
  - Reads joystick (A0) + button (D2).
  - Provides debounced `shortPress` and `longPress`, and `up/down` flags.

### Optional PC side

- **`main.cpp`**
  - Console test harness (W/S to move, Enter/Space to select, Q to quit).
- **`PcHighscoreStore.h`**
  - In-memory implementation of `IHighscoreStore`.

---

## 3. Hardware Connections (Arduino)

### LCD (16x2 HD44780)

Configured as:

```cpp
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
```

- LCD `RS` → D9  
- LCD `E`  → D8  
- LCD `D4` → D7  
- LCD `D5` → D6  
- LCD `D6` → D5  
- LCD `D7` → D4  
- `VSS` → GND, `VDD` → 5V  
- `VO`  → contrast pot wiper (ends to 5V and GND)  
- Backlight: `A` → 5V through ~220 Ω, `K` → GND  

### Joystick

- `VCC` → 5V  
- `GND` → GND  
- `VRy` → A0 (vertical axis, `JOY_V_PIN`)  
- `SW`  → D2 (`JOY_BTN_PIN`, `INPUT_PULLUP`)  
- `VRx` can be left unused.

### Buzzer

- Buzzer `+` → 220 Ω → D3 (`BUZZER_PIN`)  
- Buzzer `−` → GND  

Used with:

```cpp
tone(BUZZER_PIN, 700, 120); // life lost
tone(BUZZER_PIN, 300, 400); // game over
```

This shorter README gives a quick overview of the game, what each file does, and how to wire the hardware.
