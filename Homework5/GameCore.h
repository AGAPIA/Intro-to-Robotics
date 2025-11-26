// GameCore.h  -- shared game logic for PC and Arduino
#pragma once
#include <stdint.h>

class RaceGame {
public:
    static constexpr int ROWS = 2;
    static constexpr int SCREEN_COLS = 16;
    static constexpr int WORLD_COLS = 24;   // 16 visible + 8 background
    static constexpr int CAR_COL = 3;    // fixed column

    // Logical content of a cell
    enum CellType : uint8_t {
        CELL_EMPTY = 0,
        CELL_OBSTACLE,
        CELL_HEART,
        CELL_COIN,
        CELL_CAR        // used only in display buffer (never stored in world[])
    };

    struct Input {
        bool moveUp = false;
        bool moveDown = false;
    };

    RaceGame() {
        rngState = 1;
        reset(3);
    }

    inline void seed(uint32_t s) {
        if (s == 0) s = 1;
        rngState = s;
    }

    inline void reset(uint8_t startLives) {
        if (startLives == 0) startLives = 1;
        if (startLives > 9)  startLives = 9;

        lives = startLives;
        score = 0;
        carRow = 0;
        running = true;

        scrollTimer = 0;
        scoreTimer = 0;
        obstacleCooldown = 4;   // guarantees some space before first obstacle

        clearWorld();
    }

    inline bool isGameOver() const { return !running; }
    inline uint8_t  getLives() const { return lives; }
    inline uint16_t getScore() const { return score; }
    inline int      getCarRow() const { return carRow; }

    // Main logic tick. deltaMs = time since last call in milliseconds.
    inline void step(uint32_t deltaMs, const Input& input) {
        if (!running) return;

        // --- Handle lane change (up/down) ---
        bool movedLane = false;
        if (input.moveUp && carRow > 0) {
            carRow--;
            movedLane = true;
        }
        else if (input.moveDown && carRow < ROWS - 1) {
            carRow++;
            movedLane = true;
        }

        if (movedLane) {
            handleCollision();   // we may have moved onto an object
            if (!running) return;
        }

        // --- Scroll world left at fixed speed ---
        scrollTimer += deltaMs;
        while (scrollTimer >= SCROLL_PERIOD_MS) {
            scrollTimer -= SCROLL_PERIOD_MS;
            scrollWorldAndGenerate();
            handleCollision();   // new column slid under the car
            if (!running) return;
        }

        // --- Time-based score: +1 every 10 seconds ---
        scoreTimer += deltaMs;
        while (scoreTimer >= SCORE_PERIOD_MS) {
            scoreTimer -= SCORE_PERIOD_MS;
            addScore(1);
        }
    }

    // Build a 2x16 buffer that includes the CAR cell.
    inline void buildWorldForDisplay(CellType out[ROWS][SCREEN_COLS]) const {
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < SCREEN_COLS; ++c) {
                out[r][c] = world[r][c];  // copy visible part [0..15]
            }
        }
        // Overlay the car
        out[carRow][CAR_COL] = CELL_CAR;
    }

    // Build the HUD row for the top line.
    // Positions:
    //  [0]   : lives digit (1-9)
    //  [1]   : 'L'
    //  [12..15] : score (0-999), right-aligned in 4 columns
    // All other positions contain ' '.
    inline void renderHUD(char hudRow[SCREEN_COLS]) const {
        for (int c = 0; c < SCREEN_COLS; ++c) {
            hudRow[c] = ' ';
        }

        // Lives: "<digit>L"
        hudRow[0] = '0' + (lives % 10);
        hudRow[1] = 'L';

        // Score 0-999, right-aligned in last 4 columns
        int base = SCREEN_COLS - 4;
        uint16_t s = score;
        hudRow[base + 3] = '0' + (s % 10);  s /= 10;
        hudRow[base + 2] = (s > 0) ? ('0' + (s % 10)) : ' ';
        s /= 10;
        hudRow[base + 1] = (s > 0) ? ('0' + (s % 10)) : ' ';
        hudRow[base + 0] = ' '; // leading space (since max is 999)
    }

private:
    // Timing and scoring constants
    static constexpr uint32_t SCROLL_PERIOD_MS = 250;    // scroll speed
    static constexpr uint32_t SCORE_PERIOD_MS = 10000;  // +1 every 10s
    static constexpr uint16_t SCORE_MAX = 999;

    CellType world[ROWS][WORLD_COLS];
    int      carRow = 0;        // lane: 0 = top, 1 = bottom
    uint8_t  lives = 3;
    uint16_t score = 0;
    bool     running = true;

    uint32_t rngState = 1;
    uint32_t scrollTimer = 0;
    uint32_t scoreTimer = 0;
    int      obstacleCooldown = 0;   // min distance between obstacles in columns

    inline void clearWorld() {
        for (int r = 0; r < ROWS; ++r)
            for (int c = 0; c < WORLD_COLS; ++c)
                world[r][c] = CELL_EMPTY;
    }

    inline uint32_t randRange(uint32_t maxExclusive) {
        // Simple LCG (portable to AVR and PC)
        rngState = 1664525u * rngState + 1013904223u;
        return rngState % maxExclusive;
    }

    inline void addScore(uint16_t amount) {
        uint32_t s = score + amount;
        if (s > SCORE_MAX) s = SCORE_MAX;
        score = static_cast<uint16_t>(s);
    }

    inline void scrollWorldAndGenerate() {
        // shift everything left
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < WORLD_COLS - 1; ++c) {
                world[r][c] = world[r][c + 1];
            }
        }
        // generate new rightmost column
        generateNewColumn(WORLD_COLS - 1);
    }

    inline void generateNewColumn(int col) {
        // default: empty
        for (int r = 0; r < ROWS; ++r)
            world[r][col] = CELL_EMPTY;

        // spacing between obstacles
        if (obstacleCooldown > 0) {
            obstacleCooldown--;
        }

        bool placedSomething = false;

        // Maybe spawn an obstacle
        if (obstacleCooldown == 0) {
            uint32_t roll = randRange(100);
            if (roll < 30) { // 30% chance
                int lane = static_cast<int>(randRange(ROWS));
                world[lane][col] = CELL_OBSTACLE;
                obstacleCooldown = 4;  // no new obstacle for at least 4 columns
                placedSomething = true;
            }
        }

        // If no obstacle, maybe spawn heart or coin
        if (!placedSomething) {
            uint32_t roll = randRange(100);
            if (roll < 10) {               // 10% heart
                int lane = static_cast<int>(randRange(ROWS));
                world[lane][col] = CELL_HEART;
            }
            else if (roll < 25) {        // 15% coin
                int lane = static_cast<int>(randRange(ROWS));
                world[lane][col] = CELL_COIN;
            }
        }
    }

    inline void handleCollision() {
        // Important: world[][] never contains CELL_CAR, only gameplay cells.
        CellType& cell = world[carRow][CAR_COL];

        switch (cell) {
        case CELL_OBSTACLE:
            if (lives > 0) {
                lives--;
            }
            cell = CELL_EMPTY;
            if (lives == 0) {
                running = false;
            }
            break;

        case CELL_HEART:
            if (lives < 9) lives++;
            cell = CELL_EMPTY;
            break;

        case CELL_COIN:
            addScore(2);   // +2 for coin
            cell = CELL_EMPTY;
            break;

        case CELL_EMPTY:
        default:
            break;
        }
    }
};
