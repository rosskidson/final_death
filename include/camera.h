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

  void Render();

 private:
  void RenderTiles();
  void KeepCameraInBounds();

  olc::PixelGameEngine* engine_ptr_;
  Vector2d position_;
  Level level_;

  //   int grid_width_;
  //   int grid_height_;
  int tile_size_;
  double viewport_width_;   // The width in tile units.
  double viewport_height_;  // The height in tile units.
  Bounds camera_bounds_;
};

}  // namespace platformer