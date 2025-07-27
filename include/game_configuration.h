#pragma once

#include "grid.h"
#include "tileset.h"

namespace platformer {

struct Tile {
  int flip_bit;
  int tile_id;
};

struct Level {
  Grid<int> property_grid;
  Grid<Tile> tile_grid;
  std::shared_ptr<TileSet> level_tileset;
};

struct GameConfiguration {
  std::unordered_map<int, std::shared_ptr<TileSet>> tilesets;
  std::vector<Level> levels;
};

}  // namespace platformer