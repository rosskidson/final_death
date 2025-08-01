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

  // Returns the camera position in tile coordinates.
  Vector2d GetCameraPosition() const;

  void RenderBackground();
  void RenderTiles();
  void RenderPlayer(const Player& player);

 private:
  void KeepCameraInBounds();

  olc::PixelGameEngine* engine_ptr_;

  // Camera position is the bottom right corner of the screen.
  // Stored in pixel coordinates, but with y up positive.
  // The decision for the camera position to be in pixel coordinates is very deliberate:
  // If the camera can move subpixel amounts, sprites not aligned to the pixel grid will
  // jiggle a pixel forwards and backwards as the camera moves.
  int cam_position_px_x_;
  int cam_position_px_y_;
  int max_cam_postion_px_x_;
  int max_cam_postion_px_y_;

  Level level_;

  int tile_size_;
  double viewport_width_;   // The width in tile units.
  double viewport_height_;  // The height in tile units.
                            //   Bounds camera_bounds_;

  olc::Sprite player_sprite_;
};

}  // namespace platformer