#include "Skin.h"
#include "core/ui/Tile.h"

char Skin::asciiFor(Tile t) const {
    switch (t) {
    case Tile::Empty:     return ' ';
    case Tile::RoadDot:   return '.';   // lane dotted marker
    case Tile::Car:       return 'A';   // car glyph on PC
    case Tile::Obstacle:  return 'X';
    case Tile::Coin:      return '$';
    case Tile::Heart:     return (char)3; // ♥ on many codepages; fallback if not
    }
    return ' ';
}

uint8_t Skin::lcdCellFor(Tile t) const {
    // Map tiles to CGRAM slots if you define custom chars on Arduino.
    // Safe defaults here (0 means standard charset or blank).
    switch (t) {
    case Tile::Empty:     return 0;
    case Tile::RoadDot:   return 2;   // pick a CGRAM slot if you have one; else render as '.'
    case Tile::Car:       return 3;
    case Tile::Obstacle:  return 4;
    case Tile::Coin:      return 5;
    case Tile::Heart:     return 6;
    }
    return 0;
}
