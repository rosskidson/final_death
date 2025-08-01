#pragma once

#include "basic_types.h"
#include "game_configuration.h"
#include "tileset.h"

namespace platformer {

class Camera {
 public:
  // The level is copied in, as opposed to a ref or pointer.
  // This class has invariants based on the level grids, so a reference could
  // result in having the rug pulled out from underneath.
  Camera(olc::PixelGameEngine* engine_ptr, Level level);

  void UpdatePosition(const Vector2d& absolute_vec);

  void MoveCamera(const Vector2d& relative_vec);

  Vector2d GetCameraPosition() const;

  void RenderBackground();
  void RenderTiles();
  void RenderPlayer(const Player& player);

 private:
  void KeepCameraInBounds();

  olc::PixelGameEngine* engine_ptr_;
  // Position is the middle of the screen.
  Vector2d position_;
  Level level_;

  int tile_size_;
  double viewport_width_;   // The width in tile units.
  double viewport_height_;  // The height in tile units.
  Bounds camera_bounds_;

  olc::Sprite player_sprite_;
};

}  // namespace platformer