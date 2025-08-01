#pragma once

#include <olcPixelGameEngine.h>

#include <vector>

using TileId = int;

namespace platformer {

class TileSet {
 public:
  TileSet() = default;
  TileSet(const std::string name, const int tileset_uid, const int width, const int height)
      : name_{name},                //
        tileset_uid_(tileset_uid),  //
        width_(width),              //
        height_(height) {           //
    tiles_.resize(width * height);  //
  }

  void SetTile(const int x, const int y, std::unique_ptr<olc::Sprite>&& sprite);
  olc::Sprite* GetTile(const int x, const int y) const;
  olc::Sprite* GetTile(const int tile_id) const;

 private:
  std::string name_;
  int tileset_uid_;
  int width_;
  int height_;
  std::vector<std::unique_ptr<olc::Sprite>> tiles_;
};

}  // namespace platformer