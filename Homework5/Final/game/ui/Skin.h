#pragma once
#include <cstdint>
#include "core/ui/Tile.h"

struct Skin {
    // ASCII for PC/console view
    char     asciiFor(Tile t) const;
    // CGRAM index for Arduino LCD (0..7 typically). Kept stable for portability.
    uint8_t  lcdCellFor(Tile t) const;
};
