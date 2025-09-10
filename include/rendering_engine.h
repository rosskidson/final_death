#pragma once

#include "basic_types.h"
#include "game_configuration.h"

namespace platformer {

// This abstracts away all pixel math with regards to sprites, drawing, etc.
class RenderingEngine {
 public:
  // The level is copied in, as opposed to a ref or pointer.
  // This class has invariants based on the level grids, so a reference could
  // result in having the rug pulled from its feet.
  RenderingEngine(olc::PixelGameEngine* engine_ptr, Level level);

  [[nodiscard]] Vector2d GetCameraPosition() const;
  void SetCameraPosition(const Vector2d& absolute_vec);
  void MoveCamera(const Vector2d& relative_vec);

  // Move the camera to keep it focused on the player.
  // The screen ratio is how much of a buffer from the player to the side of the screen
  // as a fraction of the screen size.
  void KeepPlayerInFrame(const Player& player, double screen_ratio);

  void RenderBackground();
  void RenderTiles();

  // TODO:: add const if you can fix the non const olc::Sprite* nonsense!!!
  void RenderPlayer(Player& player);

 private:
  void KeepCameraInBounds();

  olc::PixelGameEngine* engine_ptr_;

  // Camera position is the bottom right corner of the screen.
  // Stored in pixel coordinates, but with y up positive.
  // The decision for the camera position to be in pixel coordinates is very deliberate:
  // If the camera can move subpixel amounts, sprites not aligned to the pixel grid will
  // jiggle about as the camera moves.
  int cam_position_px_x_;
  int cam_position_px_y_;
  int max_cam_postion_px_x_;
  int max_cam_postion_px_y_;

  Level level_;

  int tile_size_;
  double viewport_width_;   // The width in tile units.
  double viewport_height_;  // The height in tile units.
};

}  // namespace platformer