#pragma once

#include "grid.h"
#include "tileset.h"

namespace platformer {

struct Tile {
  int flip_bit;
  int tile_id;
};

struct Level {
  Grid<int> property_grid;                 // Solid, water, lava etc.
  Grid<Tile> tile_grid;                    // Which tiles to draw.
  std::shared_ptr<TileSet> level_tileset;  // Tileset for this level.
};

struct GameConfiguration {
  std::unordered_map<int, std::shared_ptr<TileSet>> tilesets;
  std::vector<Level> levels;
};

}  // namespace platformer