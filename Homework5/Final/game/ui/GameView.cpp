#include "GameView.h"
#include <algorithm>

static inline int clampi(int v, int lo, int hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void GameView::clear(TileCanvas2x16& c) {
    for (int y = 0; y < c.height(); ++y)
        for (int x = 0; x < c.width(); ++x)
            c.set(x, y, Tile::Empty);
}

// 2x2 bitmap digits using Obstacle as "on" pixel.
// Each digit is 2 columns wide and 2 rows tall (rows 0..1).
// Indexing: bits 0..3 map to (x=0,y=0),(x=1,y=0),(x=0,y=1),(x=1,y=1).
static const uint8_t DIG2x2[10] = {
    /*0*/ 0b1111, /*1*/ 0b0110, /*2*/ 0b1101, /*3*/ 0b1110, /*4*/ 0b0111,
    /*5*/ 0b1011, /*6*/ 0b1011, /*7*/ 0b0110, /*8*/ 0b1111, /*9*/ 0b1111
};

static void drawDigit2x2(TileCanvas2x16& c, int colStart, int digit) {
    digit = std::max(0, std::min(9, digit));
    uint8_t b = DIG2x2[digit];
    // top row y=0, bottom y=1
    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            bool on = (b >> (dy * 2 + dx)) & 1;
            c.set(colStart + dx, dy, on ? Tile::Obstacle : c.get(colStart + dx, dy));
        }
    }
}

void GameView::drawHUD(const CoreGame& game, TileCanvas2x16& c) {
    const int W = c.width();

    // Lives in the top-left corner (row 0, cols 0..2)
    int hearts = std::max(0, std::min(game.lives(), 3));
    for (int i = 0; i < hearts && i < 3 && i < W; ++i) c.set(i, 0, Tile::Heart);

    // Numeric score 1..100 on the top-right as digits (2 columns per digit)
    int score = std::max(1, std::min(100, game.score()));
    int d0 = score % 10;
    int d1 = (score / 10) % 10;
    int d2 = (score / 100) % 10;

    // rightmost three 2-col slots: [10..11]=hundreds, [12..13]=tens, [14..15]=ones
    if (W >= 16) {
        // clear those columns first (keep background)
        for (int x = 10; x < 16; ++x) c.set(x, 0, c.get(x, 0));

        if (d2 > 0) drawDigit2x2(c, 10, d2);  // show hundreds only if non-zero
        drawDigit2x2(c, 12, d1);
        drawDigit2x2(c, 14, d0);
    }
}

void GameView::drawPlaying(const CoreGame& game, TileCanvas2x16& c) {
    // Background: dotted road everywhere
    for (int y = 0; y < c.height(); ++y)
        for (int x = 0; x < c.width(); ++x)
            c.set(x, y, Tile::RoadDot);

    // HUD overlays (row 0 corners)
    drawHUD(game, c);

    // Entities on both rows; avoid overwriting HUD cells on row 0
    auto isHudTop = [](int x) { return (x >= 0 && x <= 2) || (x >= 10 && x <= 15); };
    for (const auto& e : game.entities()) {
        int rr = clampi(e.row, 0, c.height() - 1);
        int cc = clampi(e.col, 0, c.width() - 1);
        if (rr == 0 && isHudTop(cc)) continue; // protect HUD cells
        c.set(cc, rr, e.kind);
    }

    // Car last
    int cr = clampi(game.carRow(), 0, c.height() - 1);
    int cc = clampi(game.carCol(), 0, c.width() - 1);
    // also protect top HUD cells if the car ever moves to row 0
    if (!(cr == 0 && (cc <= 2 || cc >= 10))) {
        c.set(cc, cr, Tile::Car);
    }
}

void GameView::drawPaused(TileCanvas2x16& c) {
    clear(c);
    for (int x = 1; x < c.width(); x += 2) c.set(x, 0, Tile::RoadDot);
    c.set(c.width() / 2, 1, Tile::Car);
}

void GameView::drawHighScore(const CoreGame& game, TileCanvas2x16& c) {
    clear(c);
    // Show high score using same 2x2 digits on the right
    int score = std::max(1, std::min(100, game.score()));
    drawHUD(game, c); // reuses digit drawing logic
    // small decoration on bottom row
    for (int x = 2; x < c.width(); x += 4) c.set(x, 1, Tile::Heart);
}

void GameView::drawGameOver(const CoreGame& game, TileCanvas2x16& c) {
    clear(c);
    // fence on top, car + digits on bottom-ish look
    for (int x = 0; x < c.width(); ++x) c.set(x, 0, Tile::Obstacle);
    c.set(c.width() / 2, 1, Tile::Car);
    drawHUD(game, c); // keeps consistent digits + hearts
}

void GameView::render(const CoreGame& game, TileCanvas2x16& canvas, const Skin&) {
    switch (game.state()) {
    case CoreGame::State::Playing:   drawPlaying(game, canvas);   break;
    case CoreGame::State::GameOver:  drawGameOver(game, canvas);  break;
    }
}
