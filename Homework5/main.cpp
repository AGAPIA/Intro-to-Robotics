#ifdef GAME_ON_PC

#include <iostream>
#include <chrono>
#include <thread>
#include <conio.h>
#include "GameController.h"
#include "PcHighscoreStore.h"

static char cellToChar(RaceGame::CellType c) {
    switch (c) {
    case RaceGame::CELL_OBSTACLE: return 'X';
    case RaceGame::CELL_HEART:    return 'L';
    case RaceGame::CELL_COIN:     return '$';
    case RaceGame::CELL_CAR:      return 'D';
    case RaceGame::CELL_EMPTY:
    default:                      return '.';
    }
}

int main() {
    PcHighscoreStore store;
    GameController   controller(store);

    controller.begin();
    controller.seedRandom(static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()
        ));

    using clock = std::chrono::steady_clock;
    auto last = clock::now();

    while (true) {
        auto now = clock::now();
        uint32_t delta = (uint32_t)
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        last = now;

        // --------- Input ----------
        GameController::Input in;
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'w' || ch == 'W') in.up = true;
            else if (ch == 's' || ch == 'S') in.down = true;
            else if (ch == '\r' || ch == ' ') in.select = true;
            else if (ch == 'q' || ch == 'Q') return 0;
        }

        controller.update(delta, in);

        // --------- Rendering ----------
        system("cls");
        auto st = controller.getState();

        if (st == GameController::STATE_PLAYING) {
            RaceGame::CellType world[RaceGame::ROWS][RaceGame::SCREEN_COLS];
            char hud[RaceGame::SCREEN_COLS];
            controller.getDisplayBuffers(world, hud);

            for (int r = 0; r < RaceGame::ROWS; ++r) {
                char line[RaceGame::SCREEN_COLS + 1];
                for (int c = 0; c < RaceGame::SCREEN_COLS; ++c) {
                    char ch = cellToChar(world[r][c]);
                    if (r == 0 && hud[c] != ' ') ch = hud[c];
                    line[c] = ch;
                }
                line[RaceGame::SCREEN_COLS] = '\0';
                std::cout << line << '\n';
            }
        }
        else if (st == GameController::STATE_MENU) {
            uint8_t idx = controller.getMenuIndex();
            std::cout << "RACE GAME\n";
            std::cout << (idx == 0 ? "> Start\n" : "  Start\n");
            std::cout << (idx == 1 ? "> Highscores\n" : "  Highscores\n");
        }
        else if (st == GameController::STATE_HISCORES) {
            uint16_t hs[3]; controller.getHighscores(hs);
            std::cout << "Highscores:\n";
            std::cout << "1: " << hs[0] << "\n";
            std::cout << "2: " << hs[1] << "\n";
            std::cout << "3: " << hs[2] << "\n";
            std::cout << "(ENTER to go back)\n";
        }
        else if (st == GameController::STATE_GAMEOVER) {
            std::cout << "GAME OVER, score = "
                << controller.getLastScore()
                << "\n(ENTER to go back to menu)\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

#endif