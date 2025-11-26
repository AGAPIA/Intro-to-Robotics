// EepromHighscoreStore.h
#pragma once

#ifndef GAME_ON_PC
#include <EEPROM.h>
#include "GameController.h"

struct EepromHighscoreStore : public IHighscoreStore {
    // layout: 0-1 magic, 2-7 three uint16_t scores
    void loadHighscores(uint16_t scores[3]) override {
        uint16_t magic;
        EEPROM.get(0, magic);
        if (magic != 0xBEEF) {
            for (int i = 0; i < 3; ++i) scores[i] = 0;
            saveHighscores(scores); // write default
            return;
        }
        for (int i = 0; i < 3; ++i) {
            EEPROM.get(2 + i * sizeof(uint16_t), scores[i]);
        }
    }

    void saveHighscores(const uint16_t scores[3]) override {
        uint16_t magic = 0xBEEF;
        EEPROM.put(0, magic);
        for (int i = 0; i < 3; ++i) {
            EEPROM.put(2 + i * sizeof(uint16_t), scores[i]);
        }
    }
};
#endif
