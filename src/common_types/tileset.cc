#include "tileset.h"

#include "utils/check.h"

namespace platformer {

void TileSet::SetTile(const int x, const int y, std::unique_ptr<olc::Sprite>&& sprite) {
  RB_CHECK(x < width_ && y < height_ && x >= 0 && y >= 0);
  tiles_[y * width_ + x] = std::move(sprite);
}

olc::Sprite* TileSet::GetTile(const int x, const int y) const {
  RB_CHECK(x < width_ && y < height_ && x >= 0 && y >= 0);
  return tiles_[y * width_ + x].get();
}

olc::Sprite* TileSet::GetTile(const int tile_id) const {
  RB_CHECK(tile_id < tiles_.size() && tile_id >= 0);
  return tiles_[tile_id].get();
}

}  // namespace platformer