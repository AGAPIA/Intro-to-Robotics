// LCDRenderer.h
#pragma once
#include <Arduino.h>
#include <LiquidCrystal.h>
#include "GameCore.h"
#include <binary.h>

class LCDRenderer {
public:
    LCDRenderer(LiquidCrystal& lcdRef) : lcd(lcdRef) {}

    // Call once from setup()
    void begin() {
        lcd.begin(16, 2);

        // --- custom glyphs (indexes 0 = car, 1 = heart, 2 = coin) ---

       // Car: rotated / side-view racer 
        static byte CAR_CHAR[8] = {
            B00000,
            B00011,
            B00111,
            B11111,
            B11111,
            B00111,
            B00011,
            B00000
        };

        // Heart emoticon (no box)
        static byte HEART_CHAR[8] = {
            B00000,
            B01010,
            B11111,
            B11111,
            B01110,
            B00100,
            B00000,
            B00000
        };

        // Coin: box with a stylized '$' in the centre
        static byte COIN_CHAR[8] = {
            B11111,
            B10001,
            B10101,  // upper part of '$'
            B10111,  // middle bend
            B11101,  // lower part of '$'
            B10001,
            B11111,
            B00000
        };

        lcd.createChar(0, CAR_CHAR);
        lcd.createChar(1, HEART_CHAR);
        lcd.createChar(2, COIN_CHAR);
    }

    void renderGame(const RaceGame::CellType world[RaceGame::ROWS][RaceGame::SCREEN_COLS],
        const char hud[RaceGame::SCREEN_COLS]) {
        for (int r = 0; r < RaceGame::ROWS; ++r) {
            lcd.setCursor(0, r);
            for (int c = 0; c < RaceGame::SCREEN_COLS; ++c) {
                // HUD overlays the first row where hud[c] is not space
                if (r == 0 && hud[c] != ' ') {
                    lcd.print(hud[c]);
                    continue;
                }

                switch (world[r][c]) {
                case RaceGame::CELL_OBSTACLE: lcd.print('X');       break;
                case RaceGame::CELL_HEART:    lcd.write(byte(1));   break;
                case RaceGame::CELL_COIN:     lcd.write(byte(2));   break;
                case RaceGame::CELL_CAR:      lcd.write(byte(0));   break;
                case RaceGame::CELL_EMPTY:
                default:                      lcd.print('.');       break;
                }
            }
        }
    }

    void renderMenu(uint8_t idx) {
        lcd.setCursor(0, 0);
        lcd.print("  RACE GAME   ");
        // Pad row 0 to 16 chars to overwrite old contents
        for (int i = 14; i < 16; ++i) lcd.print(' ');

        lcd.setCursor(0, 1);
        lcd.print(idx == 0 ? '>' : ' ');
        lcd.print("Start ");
        lcd.print(idx == 1 ? '>' : ' ');
        lcd.print("Score");
        int used = 1 + 5 + 1 + 5;
        for (; used < 16; ++used) lcd.print(' ');
    }


    void renderHighscores(const uint16_t hs[3]) {
        lcd.setCursor(0, 0);
        lcd.print(" Highscores   ");
        for (int i = 14; i < 16; ++i) lcd.print(' ');

        lcd.setCursor(0, 1);
        // Build the line and pad to 16
        lcd.print("1:");
        lcd.print(hs[0]);
        lcd.print(' ');
        lcd.print("2:");
        lcd.print(hs[1]);
        lcd.print(' ');
        lcd.print("3:");
        lcd.print(hs[2]);

        // Ensure full row erase
        int used = 0;
        // Rough count: "1:" + score + space + "2:" + score + space + "3:" + score
        // We don't know exact digits here, so just pad generously:
        for (; used < 16; ++used) lcd.print(' ');
    }


    void renderGameOver(uint16_t score) {
        lcd.setCursor(0, 0);
        lcd.print("   GAME OVER  ");
        for (int i = 13; i < 16; ++i) lcd.print(' ');

        lcd.setCursor(0, 1);
        lcd.print("Score:");
        lcd.print(score);
        // Pad rest of row
        int used = 6; // "Score:" length
        if (score >= 100) used += 3;
        else if (score >= 10)  used += 2;
        else                   used += 1;
        for (; used < 16; ++used) lcd.print(' ');
    }

    void renderPause(uint8_t idx) {
        // Row 0: "PAUSED"
        lcd.setCursor(0, 0);
        lcd.print("   PAUSED     ");
        for (int i = 12; i < 16; ++i) lcd.print(' '); // pad row

        // Row 1: ">Resume  Menu" style
        lcd.setCursor(0, 1);
        lcd.print(idx == 0 ? '>' : ' ');
        lcd.print("Resume ");
        lcd.print(idx == 1 ? '>' : ' ');
        lcd.print("Menu");
        int used = 1 + 6 + 1 + 4;  // '>' + "Resume" + ' ' + "Menu"
        for (; used < 16; ++used) lcd.print(' ');
    }


private:
    LiquidCrystal& lcd;
};
