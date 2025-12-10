#pragma once
#include <cstdint>
#include "core/game/CoreGame.h"
#include "core/ui/TileCanvas.h"
#include "game/ui/Skin.h" // not strictly required here, kept for parity

class GameView {
public:
    // Render the whole frame into the given 2x16 canvas
    void render(const CoreGame& game, TileCanvas2x16& canvas, const Skin& /*skin*/);

private:
    // helpers
    void clear(TileCanvas2x16& c);
    void drawMenu(const CoreGame& game, TileCanvas2x16& c);
    void drawHUD(const CoreGame& game, TileCanvas2x16& c);
    void drawPlaying(const CoreGame& game, TileCanvas2x16& c);
    void drawPaused(TileCanvas2x16& c);
    void drawHighScore(const CoreGame& game, TileCanvas2x16& c);
    void drawGameOver(const CoreGame& game, TileCanvas2x16& c);
};
