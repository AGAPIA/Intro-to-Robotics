// InputService.h
#pragma once
#include <Arduino.h>
#include "GameController.h"

class InputService {
public:
    struct ExtendedInput {
        bool up = false;
        bool down = false;
        bool shortPress = false; // tap
        bool longPress = false; // press & hold
    };

    // joyVPin = analog pin, joyBtnPin = digital pin (INPUT_PULLUP)
    InputService(int joyVPin, int joyBtnPin,
        int deadzone = 200,
        uint16_t debounce = 50,
        int centerValue = 512,
        uint16_t longPress = 700) // ms
        : vPin(joyVPin),
        btnPin(joyBtnPin),
        deadzone(deadzone),
        debounceMs(debounce),
        center(centerValue),
        longPressMs(longPress),
        lastRawBtn(HIGH),
        stableBtn(HIGH),
        lastChangeTime(0),
        pressStartMs(0)
    {
    }

    void begin() {
        pinMode(vPin, INPUT);
        pinMode(btnPin, INPUT_PULLUP);
        lastRawBtn = digitalRead(btnPin);
        stableBtn = lastRawBtn;
        lastChangeTime = millis();
        pressStartMs = 0;
    }

    ExtendedInput read(uint32_t nowMs) {
        ExtendedInput out;

        // ---- Joystick up/down ----
        int v = analogRead(vPin);
        if (v < center - deadzone)      out.up = true;
        else if (v > center + deadzone) out.down = true;

        // ---- Button with debounce and long-press detection ----
        int raw = digitalRead(btnPin);

        if (raw != lastRawBtn) {
            lastRawBtn = raw;
            lastChangeTime = nowMs;        // start debounce window
        }

        // check if state became stable
        if ((nowMs - lastChangeTime) > debounceMs && raw != stableBtn) {
            stableBtn = raw;

            if (stableBtn == LOW) {
                // button just pressed
                pressStartMs = nowMs;
            }
            else {
                // button just released
                if (pressStartMs != 0) {
                    uint32_t duration = nowMs - pressStartMs;
                    if (duration >= longPressMs) {
                        out.longPress = true;
                    }
                    else if (duration >= 10) {
                        // treat as short press (ignore ultra-tiny glitches)
                        out.shortPress = true;
                    }
                }
                pressStartMs = 0;
            }
        }

        return out;
    }

private:
    int vPin;
    int btnPin;
    int deadzone;
    uint16_t debounceMs;
    int center;
    uint16_t longPressMs;

    int      lastRawBtn;
    int      stableBtn;
    uint32_t lastChangeTime;
    uint32_t pressStartMs;
};
