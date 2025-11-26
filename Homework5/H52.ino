#ifndef GAME_ON_PC

#include <LiquidCrystal.h>
#include "GameCore.h"
#include "GameController.h"
#include "EepromHighscoreStore.h"
#include "LCDRenderer.h"
#include "InputService.h"

// --- Pins (from your wiring) ---
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
const int JOY_V_PIN = A0;
const int JOY_BTN_PIN = 2;

const int BUZZER_PIN = 3;


// --- Platform-specific services ---
EepromHighscoreStore store;
GameController       controller(store);
LCDRenderer          renderer(lcd);
InputService         inputService(JOY_V_PIN, JOY_BTN_PIN);

// --- Render throttling and state tracking (to avoid blinking) ---
static GameController::State g_lastState = (GameController::State)(-1);
static uint32_t g_lastRenderMs = 0;
const uint32_t RENDER_PERIOD_MS = 50; // redraw at most every 50 ms

// --- Local pause state (Arduino-only, core stays unaware) ---
static bool   g_paused         = false;
static bool   g_lastPaused     = false;  // to detect pause/unpause for clear()
static uint8_t g_pauseMenuIndex = 0;     // 0 = Resume, 1 = Menu

// --- Buzzer helpers ---
void playLifeLostSound() {
    // Short high beep
    tone(BUZZER_PIN, 700, 120);   // freq=700 Hz, dur=120 ms
}

void playGameOverSound() {
    // Lower, longer beep
    tone(BUZZER_PIN, 300, 400);   // freq=300 Hz, dur=400 ms
}


// -------------------------------------------------
void setup() {
    renderer.begin();
    inputService.begin();

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    controller.begin(3);          // 3 lives
    controller.seedRandom(millis());
}

void loop() {
    static uint32_t lastMs = millis();
    uint32_t now = millis();
    uint32_t delta = now - lastMs;
    lastMs = now;

    // Read input for this frame (with long-press)
    InputService::ExtendedInput ext = inputService.read(now);

    // Current controller state before update
    GameController::State stBefore = controller.getState();

      // Track lives before this frame's update
    uint8_t livesBefore = controller.getGame().getLives();

    // ---- Enter pause on long press while playing (only if not already paused) ----
    if (stBefore == GameController::STATE_PLAYING && ext.longPress && !g_paused) {
        g_paused = true;
        g_pauseMenuIndex = 0;  // default to "Resume"
    }

    GameController::Input ctrlIn;

    if (!g_paused) {
        // Normal flow: feed up/down + short press into controller
        ctrlIn.up     = ext.up;
        ctrlIn.down   = ext.down;
        ctrlIn.select = ext.shortPress;
        controller.update(delta, ctrlIn);

        // --- Detect life lost and game over for sounds ---
        GameController::State stAfter  = controller.getState();
        uint8_t livesAfter = controller.getGame().getLives();

        // Life lost: still playing, fewer lives
        if (stBefore == GameController::STATE_PLAYING &&
            stAfter  == GameController::STATE_PLAYING &&
            livesAfter < livesBefore) {
            playLifeLostSound();
        }

        // Game over: transitioned from PLAYING to GAMEOVER
        if (stBefore == GameController::STATE_PLAYING &&
            stAfter  == GameController::STATE_GAMEOVER) {
            playGameOverSound();
        }

    } else {
        // While paused: freeze game logic (no delta, no input),
        // and handle pause menu locally
        GameController::Input emptyIn;
        controller.update(0, emptyIn);   // keep state in PLAYING

        // Navigate pause menu: up -> Resume, down -> Menu
        if (ext.up)   g_pauseMenuIndex = 0;
        if (ext.down) g_pauseMenuIndex = 1;

        // Short press: activate selected option
        if (ext.shortPress) {
            if (g_pauseMenuIndex == 0) {
                // Resume
                g_paused = false;
            } else {
                // Quit to main menu: reset controller
                controller.begin(3);  // back to MENU with 3 lives
                g_paused = false;
            }
        }

    }


    // Render according to current state, but:
    //  - only when state changes OR pause toggles
    //  - and at most every RENDER_PERIOD_MS
    GameController::State st = controller.getState();
    bool stateChanged = (st != g_lastState) || (g_paused != g_lastPaused);
    uint32_t sinceLastRender = now - g_lastRenderMs;

    if (stateChanged) {
        // Clear once when entering a new "render mode"
        lcd.clear();
    }

    if (stateChanged || sinceLastRender >= RENDER_PERIOD_MS) {
        if (st == GameController::STATE_PLAYING) {
            if (g_paused) {
                // Show pause menu overlay instead of game
                renderer.renderPause(g_pauseMenuIndex);
            } else {
                // Normal game rendering
                RaceGame::CellType world[RaceGame::ROWS][RaceGame::SCREEN_COLS];
                char hud[RaceGame::SCREEN_COLS];
                controller.getDisplayBuffers(world, hud);
                renderer.renderGame(world, hud);
            }
        }
        else if (st == GameController::STATE_MENU) {
            renderer.renderMenu(controller.getMenuIndex());
        }
        else if (st == GameController::STATE_HISCORES) {
            uint16_t hs[3];
            controller.getHighscores(hs);
            renderer.renderHighscores(hs);
        }
        else if (st == GameController::STATE_GAMEOVER) {
            renderer.renderGameOver(controller.getLastScore());
        }

        g_lastState    = st;
        g_lastPaused   = g_paused;
        g_lastRenderMs = now;
    }


}

#endif  