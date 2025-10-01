#pragma once

#include <memory>
#include <optional>

#include "animated_sprite.h"
#include "basic_types.h"
#include "game_configuration.h"
#include "player.h"
#include "utils/parameter_server.h"

namespace platformer {

// This abstracts away all pixel math with regards to sprites, drawing, etc.
class RenderingEngine {
 public:
  // The level is copied in, as opposed to a ref or pointer.
  // This class has invariants based on the level grids, so a reference could
  // result in having the rug pulled from its feet.
  RenderingEngine(olc::PixelGameEngine* engine_ptr,
                  Level level,
                  std::shared_ptr<ParameterServer> parameter_server);

  [[nodiscard]] Vector2d GetCameraPosition() const;
  void SetCameraPosition(const Vector2d& absolute_vec);
  void MoveCamera(const Vector2d& relative_vec);

  // Move the camera to keep it focused on the player.
  // The screen ratio is how much of a buffer from the player to the side of the screen
  // as a fraction of the screen size.
  void KeepPlayerInFrame(const Player& player);

  void RenderBackground();
  void RenderForeground();
  void RenderTiles();
  void RenderPlayer(Player& player);

  // Add a background layer to render.
  // Backgrounds will be rendered in the order that they are added.
  // A scroll slowdown factor of 2 moves the image at half the speed of the camera.
  // This could be made a floating value if need be.
  [[nodiscard]] bool AddBackgroundLayer(const std::filesystem::path& background_png,
                                        double scroll_slowdown_factor);

  // Add a foreground layer to render.
  // Same as background but it is rendered after the tiles.
  [[nodiscard]] bool AddForegroundLayer(const std::filesystem::path& background_png,
                                        double scroll_slowdown_factor);

  // Add a solid color to draw before any background layers.
  // Use this either if 1) you don't have a background or
  //                    2) All your background layers have transparency.
  void AddFoundationBackgroundLayer(uint8_t r, uint8_t g, uint8_t b);

 private:
  struct BackgroundLayer {
    std::unique_ptr<olc::Sprite> background_img;
    double scroll_slowdown_factor;
  };

  void KeepCameraInBounds();
  void RenderBackgroundLayer(const BackgroundLayer& background_layer);

  olc::PixelGameEngine* engine_ptr_;

  std::shared_ptr<ParameterServer> parameter_server_;

  // std::unique_ptr<olc::Sprite> background_;
  std::vector<BackgroundLayer> background_layers_;
  std::vector<BackgroundLayer> foreground_layers_;
  std::optional<olc::Pixel> foundation_background_color_;

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