// PcHighscoreStore.h
#pragma once
#include "GameController.h"

struct PcHighscoreStore : public IHighscoreStore {
    uint16_t scores[3] = { 0, 0, 0 };

    void loadHighscores(uint16_t out[3]) override {
        for (int i = 0; i < 3; ++i) out[i] = scores[i];
    }

    void saveHighscores(const uint16_t in[3]) override {
        for (int i = 0; i < 3; ++i) scores[i] = in[i];
    }
};