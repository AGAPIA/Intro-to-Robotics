// GameController.h  -- shared between PC and Arduino
#pragma once
#include <stdint.h>
#include "GameCore.h"

// Platform-specific storage hook
struct IHighscoreStore {
    virtual void loadHighscores(uint16_t scores[3]) = 0;
    virtual void saveHighscores(const uint16_t scores[3]) = 0;
    virtual ~IHighscoreStore() {}
};

class GameController {
public:
    enum State {
        STATE_MENU,
        STATE_PLAYING,
        STATE_HISCORES,
        STATE_GAMEOVER
    };

    struct Input {
        bool up = false;
        bool down = false;
        bool select = false;
    };

    GameController(IHighscoreStore& storeRef)
        : store(storeRef)
    {
        highscores[0] = highscores[1] = highscores[2] = 0;
        state = STATE_MENU;
        menuIndex = 0;
        lastScore = 0;
        gameOverTimer = 0;
    }

    void begin(uint8_t startingLives = 3) {
        store.loadHighscores(highscores);
        game.reset(startingLives);
        state = STATE_MENU;
        menuIndex = 0;
        gameOverTimer = 0;
    }

    void seedRandom(uint32_t s) { game.seed(s); }

    void update(uint32_t deltaMs, const Input& in) {
        switch (state) {
        case STATE_MENU:
            handleMenu(in);
            break;
        case STATE_HISCORES:
            handleHighscores(in);
            break;
        case STATE_PLAYING:
            handlePlaying(deltaMs, in);
            break;
        case STATE_GAMEOVER:
            handleGameOver(deltaMs, in);
            break;
        }
    }

    // ----- Read-only view for renderers -----
    State   getState()     const { return state; }
    uint8_t getMenuIndex() const { return menuIndex; }
    uint16_t getLastScore() const { return lastScore; }

    void getHighscores(uint16_t out[3]) const {
        for (int i = 0; i < 3; ++i) out[i] = highscores[i];
    }

    // Build buffers for the PLAYING state (world + HUD).
    void getDisplayBuffers(RaceGame::CellType world[RaceGame::ROWS][RaceGame::SCREEN_COLS],
        char hud[RaceGame::SCREEN_COLS]) const
    {
        game.buildWorldForDisplay(world);
        game.renderHUD(hud);
    }

    const RaceGame& getGame() const { return game; }

private:
    RaceGame        game;
    IHighscoreStore& store;

    State    state;
    uint8_t  menuIndex;
    uint16_t highscores[3];
    uint16_t lastScore;
    uint32_t gameOverTimer; // ms in GAMEOVER state

    // ----- State handlers -----

    void handleMenu(const Input& in) {
        if (in.up)   menuIndex = 0;
        if (in.down) menuIndex = 1;

        if (in.select) {
            if (menuIndex == 0) {
                game.reset(3);
                state = STATE_PLAYING;
            }
            else {
                state = STATE_HISCORES;
            }
        }
    }

    void handleHighscores(const Input& in) {
        if (in.select) {
            state = STATE_MENU;
        }
    }

    void handlePlaying(uint32_t deltaMs, const Input& in) {
        RaceGame::Input gIn;
        gIn.moveUp = in.up;
        gIn.moveDown = in.down;

        game.step(deltaMs, gIn);

        if (game.isGameOver()) {
            lastScore = game.getScore();
            insertScore(lastScore);
            store.saveHighscores(highscores);
            gameOverTimer = 0;
            state = STATE_GAMEOVER;
        }
    }

    void handleGameOver(uint32_t deltaMs, const Input& in) {
        gameOverTimer += deltaMs;
        if (in.select || gameOverTimer > 3000) {
            state = STATE_MENU;
        }
    }

    void insertScore(uint16_t s) {
        for (int i = 0; i < 3; ++i) {
            if (s > highscores[i]) {
                for (int j = 2; j > i; --j) highscores[j] = highscores[j - 1];
                highscores[i] = s;
                break;
            }
        }
    }
};