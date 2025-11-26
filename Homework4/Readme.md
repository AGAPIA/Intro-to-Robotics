# Simon Says — Arduino UNO + 74HC595 + Joystick + 4×7-Seg (CC)

## Demo & Code
- 🎥 Video: [![Watch demo](thumb.png)](https://youtu.be/5UD2rDWRh00)

## Overview
Non-blocking “Simon Says” memory game on a 4-digit common-cathode display. A 74HC595 drives segments (Q7→A … Q0→DP); digits are multiplexed from GPIO. Input: joystick (X/Y + SW) + Play/Stop button.  
States: IDLE_MENU → SHOW_SEQ → INPUT → CHECK → RESULT. Difficulty increases by shrinking display time.

## Hardware
- UNO, 74HC595, 4-digit 7-seg (CC), 8× 220–330 Ω (A–G, DP), joystick, 1× button.
- 74HC595: DS=D12, SHCP=D10, STCP=D11; Q7..Q0 → A,B,C,D,E,F,G,DP (via resistors).
- Digits (active LOW): D7 (leftmost), D6, D5, D4 (rightmost).
- Joystick: VRx=A1, VRy=A2, SW=D8 (INPUT_PULLUP).
- Play/Stop: D3 (INPUT_PULLUP). Decouple 74HC595 with 0.1 µF.

## Controls
- Menu: Left/Right selects PLAY / SCOR / STOP; SW short or Play short confirms.
- Input: Left/Right moves cursor; Up/Down changes symbol; SW short locks; SW long or Play short submits.
- Play long (in game) → STOP/menu.

## Timing
Multiplex ≈ 1200 µs/digit (~208 Hz per digit). Non-blocking with `millis`/`micros`. Cursor blinks ~150 ms.

## Scoring
On success: `score++`, `highScore` updated, `showTimeMs` decreases (min 800 ms).  
`USE_FIXED_SEQUENCE` enables a fixed test sequence for debugging.

## Build
UNO, Serial 115200. Verify **common-cathode** and the Q7→A … Q0→DP mapping.
