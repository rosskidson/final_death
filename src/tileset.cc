#include "tileset.h"

#include "utils/logging.h"

#undef NDEBUG
#include <cassert>

namespace platformer {

void TileSet::SetTile(const int x, const int y, std::unique_ptr<olc::Sprite>&& sprite) {
  assert(x < width_ && y < height_ && x >= 0 && y >= 0);
  tiles_[y * width_ + x] = std::move(sprite);
}

olc::Sprite* TileSet::GetTile(const int x, const int y) const {
  LOG_INFO(x << " " << y << " " << width_ << " " << height_);
  assert(x < width_ && y < height_ && x >= 0 && y >= 0);
  return tiles_[y * width_ + x].get();
}

olc::Sprite* TileSet::GetTile(const int tile_id) const {
  assert(tile_id < tiles_.size() && tile_id >= 0);
  return tiles_[tile_id].get();
}

}  // namespace platformer