# Autonomous Raspberry Pi Obstacle-Avoiding Car

## Overview

This project implements a small two-wheel robot car based on a Raspberry Pi that can drive forward and avoid obstacles using distance sensors. The goal is to build a simple autonomous system that makes its own decisions about motion (forward, turn, stop) based only on onboard sensors.

The same repository will contain:

- Hardware schematics / wiring diagram
- Python code for the Raspberry Pi
- Notes on tuning and limitations

---

## Features

- Differential-drive robot (two DC motors + caster wheel)
- Raspberry Pi as main controller (Python)
- Ultrasonic distance sensor for obstacle detection
- Optional IR sensors for additional obstacle or edge detection
- Simple navigation logic:
  - Drive forward by default
  - Slow down or stop when an obstacle is too close
  - Turn left or right to go around obstacles
- Basic logging (distances and motor commands) for debugging and tuning
- Top-level script for running a demo without manual control

---

## Bill of Materials (BOM)

Electronics:

- 1 × Raspberry Pi (3B / 3B+ / 4) with microSD card (≥ 16 GB)
- 1 × Dual DC motor driver (e.g., L298N or TB6612FNG)
- 2 × DC gear motors with wheels
- 1 × Caster wheel (mechanical support)
- 1 × Ultrasonic distance sensor (HC-SR04 or similar)
- (Optional) 2–3 × IR sensors (obstacle or line detection)
- 1 × Battery pack / power bank (for Raspberry Pi and/or motors)
- Jumper wires (male–male, male–female)
- 1 × Small breadboard

Mechanical:

- 1 × Two-wheel robot chassis
- Standoffs, screws, spacers for mounting the Raspberry Pi and sensors

No ESP32 is required for the core project. All control runs on the Raspberry Pi.

---

## System Architecture

### System Boundary (Q1)

Inside the system:

- Raspberry Pi and its software
- Motor driver and motors
- Distance sensors (ultrasonic and optional IR)
- Robot chassis and battery

Outside the system:

- Environment (floor, walls, obstacles)
- Human operator who starts/stops the demo and positions obstacles
- Any laptop used for development or SSH

The robot must decide its motion based only on onboard sensors during the demo.

### Where Intelligence Lives (Q2)

All decision-making runs on the Raspberry Pi:

- Reading sensors (distance, optional IR)
- Simple perception (e.g., “obstacle ahead within X cm”)
- Navigation logic (state machine or behavior rules)
- Motor commands (speed and direction for each wheel)

There is no external cloud or PC doing real-time control.

### Hardest Technical Problem (Q3)

The technically hardest part is achieving robust navigation with simple sensors:

- Ultrasonic sensors have noise, blind spots and reflections.
- Fixed thresholds do not always work in all environments.
- It is easy for the robot to oscillate or get stuck in corners.

Most of the work will go into tuning distance thresholds, turning angles and speeds so that the robot can move smoothly and avoid collisions in a typical corridor or room.

---

## Minimum Demo (Q4)

The minimum demo target:

- Place the robot at a starting point on the floor.
- Place 2–3 static obstacles (e.g., boxes) in front of its path.
- Start the program and do not touch the robot.
- The robot drives forward, detects obstacles, and:
  - Slows or stops when something is too close.
  - Turns left or right to go around the obstacle.
  - Continues moving for at least 30–60 seconds without collisions.

If this runs reliably, the basic project goal is achieved.

---

## Why This Is Not Just a Tutorial (Q5)

The project is based on common building blocks (motor driver control and distance sensors), but it is not a copy of a single tutorial. The main differences are:

- The code is organized into clear layers:
  - hardware abstraction for motors and sensors,
  - navigation logic,
  - main script that ties everything together.
- Parameters such as speed, safe distance and turning duration are tuned experimentally and documented.
- The repository will include notes about failure cases, limitations and possible extensions (e.g., adding a scanning sensor or more sensors around the robot).

The result is a small autonomous robot with its own structure and documentation.

---

## Connections (Planned)

The exact pins may change, but the approximate wiring is:

- Motor driver:
  - Motor A/B outputs → left and right DC motors
  - IN1, IN2 (and IN3, IN4) → Raspberry Pi GPIO for direction
  - ENA/ENB → Raspberry Pi GPIO or hardware PWM
  - Motor power supply → battery pack
  - Logic supply → 5 V (or Pi 5 V, depending on driver)
- Ultrasonic sensor (HC-SR04):
  - VCC → 5 V
  - GND → GND
  - TRIG → Raspberry Pi GPIO
  - ECHO → Raspberry Pi GPIO (possibly via voltage divider to 3.3 V)
- Optional IR sensors:
  - VCC → 5 V
  - GND → GND
  - OUT → Raspberry Pi GPIO

The exact pin map will be documented in the repository once the wiring is finalized.

---

## Repository Structure (planned)

- `src/`
  - `motors.py` – motor driver abstraction
  - `sensors.py` – ultrasonic and IR sensor reading
  - `controller.py` – navigation logic / state machine
  - `main.py` – entry point for the demo
- `docs/`
  - wiring diagram
  - short design notes
- `README.md` – project overview, hardware list and setup instructions
