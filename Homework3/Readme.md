\# Home Alarm System — Arduino UNO (Ultrasonic + LDR + LEDs + Buzzer)



\## Demo \& Code

\- 🎥 Video: \[YouTube title here](https://youtu.be/uOdt9c44fSI)

\- 💻 Source code: \[GitHub repo](https://github.com/AGAPIAhttps://github.com/AGAPIA/Introduction-to-robotics/Homework3)



\## Overview

Non-blocking alarm with ultrasonic intrusion, LDR night auto-arm, buzzer, and dual LEDs.

States: DISARMED → ARMING (3 s) → ARMED → INTRUSION\_PENDING (3 s) → ALARMING.

Serial menu with password-gated disarm and runtime settings.



\## Hardware

\- UNO, HC-SR04, LDR + 10 kΩ divider, buzzer, 2× LEDs + 220–330 Ω.

\- HC-SR04: TRIG=D7, ECHO=D4.

\- LEDs (to GND via resistor): Green=D6 (PWM), Red=D10 (PWM).

\- Buzzer: D9 (+) → buzzer, GND (−).

\- LDR divider: 5 V — LDR — A0 — 10 kΩ — GND.



\## Behavior

\- DISARMED: solid green; ARMING: fast green; ARMED: slow red heartbeat.

\- INTRUSION\_PENDING: fast red (3 s grace); ALARMING: rapid red + buzzer.

\- Auto-arm when A0 < ldrThreshold; intrusion when |distance − baseline| ≥ ultrasonicDeltaCm.

\- Baseline = averaged samples after warmup.



\## Serial UI (115200)

DISARMED: \[1] Arm, \[2] Test alarm, \[3] Settings.

Armed/Alarming: \[D] Disarm (password).

Settings: set ultrasonic delta, LDR threshold, buzzer tone, system name, change password.



\## Test Modes

`RUN\_TEST\_MODE`: 1=LEDs, 2=Buzzer, 3=Ultrasonic (cm), 4=LDR (A0), 0=Normal.



\## Notes

All effects are non-blocking (millis/micros). Tune thresholds in `Settings cfg`.



