\# Homework #2 — Traffic Lights with 7-Segment Countdown (Arduino)



This assignment implements a deterministic, \*\*finite-state machine (FSM)\*\* traffic controller on an Arduino.  

The controller coordinates \*\*car LEDs\*\*, \*\*pedestrian LEDs\*\*, an acoustic \*\*buzzer\*\*, and a \*\*common-cathode 7-segment\*\* display that shows a live countdown during pedestrian phases.



\## Demo \& Code

\- 🎥 Video: \[!\[Watch demo](thumb.png)](https://youtu.be/nCo7Qcdx0ys)



---



\## 1) Hardware



\- \*\*MCU:\*\* Arduino Uno (or compatible)  

\- \*\*LEDs for Cars:\*\* Green, Yellow, Red (either three discrete LEDs or one RGB LED with separate series resistors for G/Y/R channels)  

\- \*\*LEDs for Pedestrians:\*\* Red, Green (discrete)  

\- \*\*7-segment display:\*\* Common-cathode, single digit (A..G, optional DP), \*\*active-HIGH\*\* segments  

\- \*\*Buzzer:\*\* Digital (ON/OFF)  

\- \*\*Push Button:\*\* With internal pull-up (wired to ground when pressed)  

\- \*\*Resistors:\*\*  

&nbsp; - One series resistor per 7-segment segment (A..G and optionally DP), ~220–330 Ω  

&nbsp; - One series resistor per LED channel (car G/Y/R and ped R/G)



\### Pin map (as used in the reference code)



| Function                               | Arduino Pin |

|----------------------------------------|-------------|

| Button (active-low, `INPUT\_PULLUP`)    | 2           |

| Buzzer                                 | 11          |

| Car Green                              | A0          |

| Car Yellow                             | A1          |

| Car Red                                | A2          |

| Ped Red                                | A3          |

| Ped Green                              | A4          |

| 7-seg A                                | 12          |

| 7-seg B                                | 10          |

| 7-seg C                                | 9           |

| 7-seg D                                | 8           |

| 7-seg E                                | 7           |

| 7-seg F                                | 6           |

| 7-seg G                                | 5           |

| 7-seg DP (optional)                    | 4           |



> \*\*Common-cathode note.\*\* Tie the display’s \*\*CC\*\* to \*\*GND\*\*. Each segment pin (A..G, DP) goes through its own resistor to the corresponding Arduino pin. Segments light when driven \*\*HIGH\*\*.



---



\## 2) FSM and Timings



States and durations (ms; configurable in code):



\- \*\*Idle\*\* — Cars \*\*Green\*\*, Peds \*\*Red\*\*.  

&nbsp; \*Press Button\* → \*\*WaitingDelay\*\*.

\- \*\*WaitingDelay\*\* — approach delay: \*\*8000\*\* → \*\*CarsYellow\*\*.

\- \*\*CarsYellow\*\* — cars \*\*Yellow\*\*, peds \*\*Red\*\*: \*\*3000\*\* → \*\*PedsGreen\*\*.

\- \*\*PedsGreen\*\* — cars \*\*Red\*\*, peds \*\*Green\*\*: \*\*8000\*\*, buzzer toggles every \*\*500\*\* ms → \*\*PedsBlink\*\*.

\- \*\*PedsBlink\*\* — cars \*\*Red\*\*, peds \*\*Green blinking\*\*: \*\*4000\*\*, blink every \*\*250\*\* ms, buzzer every \*\*200\*\* ms → \*\*Idle\*\*.



The \*\*7-segment\*\* shows seconds remaining in `CarsYellow`, `PedsGreen`, and `PedsBlink`.  

In `Idle` and `WaitingDelay` the display is off.



\*\*Debounce:\*\* button ISR applies a 50 ms guard window.



---



\## 3) Operation



1\. Power up: system starts in \*\*Idle\*\* (Cars=Green, Peds=Red).  

2\. \*\*Press the button\*\* → \*\*WaitingDelay\*\* (keeps Idle lights for the configured approach time).  

3\. The sequence advances automatically: \*\*CarsYellow → PedsGreen → PedsBlink → Idle\*\*.  

4\. During pedestrian phases, the \*\*buzzer\*\* aids crossing; the \*\*7-segment\*\* shows remaining seconds.



---



\## 4) Build \& Upload



\- Open the provided sketch, verify pin map matches wiring.  

\- Select your board/port, \*\*Compile\*\* and \*\*Upload\*\*.





