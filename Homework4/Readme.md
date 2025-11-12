\# Simon Says — Arduino UNO + 74HC595 + Joystick + 4×7-Seg (CC)



\## Demo \& Code

\- 🎥 Video: \[![Watch demo](thumb.png)](https://youtu.be/5UD2rDWRh00)

\- 💻 Source code: \[GitHub repo](https://github.com/AGAPIAhttps://github.com/AGAPIA/Introduction-to-robotics/Homework4)



\## Overview

Non-blocking “Simon Says” memory game on a 4-digit common-cathode display. A 74HC595 drives segments (Q7→A … Q0→DP); digits are multiplexed from GPIO. Input: joystick (X/Y + SW) + Play/Stop button. States: IDLE\_MENU → SHOW\_SEQ → INPUT → CHECK → RESULT. Difficulty rises by shrinking display time.



\## Hardware

\- UNO, 74HC595, 4-digit 7-seg (CC), 8× 220–330 Ω (A–G, DP), joystick, 1× button.

\- 74HC595: DS=D12, SHCP=D10, STCP=D11; Q7..Q0 → A,B,C,D,E,F,G,DP (via resistors).

\- Digits (active LOW): D7 (leftmost), D6, D5, D4 (rightmost).

\- Joystick: VRx=A1, VRy=A2, SW=D8 (INPUT\_PULLUP).

\- Play/Stop: D3 (INPUT\_PULLUP). Decouple 74HC595 (0.1 µF).



\## Controls

\- Menu: Left/Right selects PLAY / SCOR / STOP; SW short or Play short confirms.

\- Input: Left/Right moves cursor; Up/Down changes symbol; SW short locks; SW long or Play short submits.

\- Play long (in game) → STOP/menu.



\## Timing

Multiplex ≈1200 µs/digit (~208 Hz per digit). Non-blocking with millis/micros. Cursor blinks ~150 ms.



\## Scoring

Success → score++, update highScore, showTimeMs decreases (min 800 ms). `USE\_FIXED\_SEQUENCE` enables a fixed test sequence for debugging.



\## Build

UNO, Serial 115200. Verify \*\*common-cathode\*\* and the Q7→A … Q0→DP mapping.



