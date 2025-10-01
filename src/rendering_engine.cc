#include "rendering_engine.h"

#include <algorithm>
#include <memory>

#include "animated_sprite.h"
#include "basic_types.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "tileset.h"
#include "utils/parameter_server.h"
#include "utils/logging.h"

namespace platformer {

constexpr double kFollowPlayerScreenRatioX = 0.4;
constexpr double kFollowPlayerScreenRatioY = 0.5;

RenderingEngine::RenderingEngine(olc::PixelGameEngine* engine_ptr,
                                 Level level,
                                 std::shared_ptr<ParameterServer> parameter_server)
    : engine_ptr_{engine_ptr},
      level_{std::move(level)},
      parameter_server_{std::move(parameter_server)} {
  parameter_server_->AddParameter(
      "rendering/follow.player.screen.ratio.x", kFollowPlayerScreenRatioX,
      "How far the player can walk towards the side of the screen before the camera follows, as a "
      "percentage of the screen size. The larger the ratio, the more centered the player will be "
      "on the screen.");
  parameter_server_->AddParameter(
      "rendering/follow.player.screen.ratio.y", kFollowPlayerScreenRatioY,
      "How far the player can walk towards the side of the screen before the camera follows, as a "
      "percentage of the screen size. The larger the ratio, the more centered the player will be "
      "on the screen.");

  const int grid_width = level_.tile_grid.GetWidth();
  const int grid_height = level_.tile_grid.GetHeight();

  tile_size_ = level_.level_tileset->GetTileSize();
  viewport_width_ = kScreenWidthPx / static_cast<double>(tile_size_);
  viewport_height_ = kScreenHeightPx / static_cast<double>(tile_size_);

  max_cam_postion_px_x_ = (grid_width * tile_size_) - kScreenWidthPx;
  max_cam_postion_px_y_ = (grid_height * tile_size_) - kScreenHeightPx;

  cam_position_px_x_ = 0;
  cam_position_px_y_ = 0;
}

void RenderingEngine::SetCameraPosition(const Vector2d& absolute_vec) {
  cam_position_px_x_ = static_cast<int>(absolute_vec.x * tile_size_);
  cam_position_px_y_ = static_cast<int>(absolute_vec.y * tile_size_);
  KeepCameraInBounds();
}

void RenderingEngine::MoveCamera(const Vector2d& relative_vec) {
  cam_position_px_x_ += static_cast<int>(relative_vec.x * tile_size_);
  cam_position_px_y_ += static_cast<int>(relative_vec.y * tile_size_);
  KeepCameraInBounds();
}

Vector2d RenderingEngine::GetCameraPosition() const {
  return Vector2d{static_cast<double>(cam_position_px_x_) / tile_size_,
                  static_cast<double>(cam_position_px_y_) / tile_size_};
}

// TODO split ratio to x and y
void RenderingEngine::KeepPlayerInFrame(const Player& player) {
  const auto screen_ratio_x =
      parameter_server_->GetParameter<double>("rendering/follow.player.screen.ratio.x");
  const auto screen_ratio_y =
      parameter_server_->GetParameter<double>("rendering/follow.player.screen.ratio.y");
  const auto position = GetCameraPosition();
  const auto convert_to_px = [&](double val) -> int { return static_cast<int>(val * tile_size_); };
  const Vector2i player_pos_px{convert_to_px(player.position.x), convert_to_px(player.position.y)};
  // Note: The y is kept to the bottom of the sprite.  Using the center causes the camera to jerk in
  // state transtions if moving up or down in the air.
  const Vector2i middle_of_player_px{
      player_pos_px.x + player.x_offset_px + player.collision_width_px / 2, player_pos_px.y};
  const int x_max_px = middle_of_player_px.x - convert_to_px(viewport_width_ * screen_ratio_x);
  const int x_min_px =
      middle_of_player_px.x - convert_to_px(viewport_width_ * (1 - screen_ratio_x));
  const int y_max_px = middle_of_player_px.y - convert_to_px(viewport_height_ * screen_ratio_y);
  const int y_min_px =
      middle_of_player_px.y - convert_to_px(viewport_height_ * (1 - screen_ratio_y));

  cam_position_px_x_ = std::max(cam_position_px_x_, x_min_px);
  cam_position_px_x_ = std::min(cam_position_px_x_, x_max_px);
  cam_position_px_y_ = std::max(cam_position_px_y_, y_min_px);
  cam_position_px_y_ = std::min(cam_position_px_y_, y_max_px);
  KeepCameraInBounds();
}

bool RenderingEngine::AddBackgroundLayer(const std::filesystem::path& background_png,
                                         double scroll_slowdown_factor) {
  BackgroundLayer layer;
  layer.scroll_slowdown_factor = scroll_slowdown_factor;
  layer.background_img = std::make_unique<olc::Sprite>();
  if (layer.background_img->LoadFromFile(background_png.string()) != olc::rcode::OK) {
    std::cout << "Failed loading background image '" << background_png << "'" << std::endl;
    return false;
  }
  background_layers_.emplace_back(std::move(layer));
  return true;
}

bool RenderingEngine::AddForegroundLayer(const std::filesystem::path& background_png,
                                         double scroll_slowdown_factor) {
  BackgroundLayer layer;
  layer.scroll_slowdown_factor = scroll_slowdown_factor;
  layer.background_img = std::make_unique<olc::Sprite>();
  if (layer.background_img->LoadFromFile(background_png.string()) != olc::rcode::OK) {
    std::cout << "Failed loading background image '" << background_png << "'" << std::endl;
    return false;
  }
  foreground_layers_.emplace_back(std::move(layer));
  return true;
}

void RenderingEngine::AddFoundationBackgroundLayer(uint8_t r, uint8_t g, uint8_t b) {
  foundation_background_color_ = olc::Pixel{r, g, b, 255};
}

void RenderingEngine::RenderBackground() {
  if (foundation_background_color_.has_value()) {
    for (int y = 0; y < kScreenHeightPx; ++y) {
      for (int x = 0; x < kScreenWidthPx; ++x) {
        engine_ptr_->Draw(x, y, *foundation_background_color_);
      }
    }
  }

  for (const auto& background_layer : background_layers_) {
    RenderBackgroundLayer(background_layer);
  }
}

void RenderingEngine::RenderForeground() {
  for (const auto& background_layer : foreground_layers_) {
    RenderBackgroundLayer(background_layer);
  }
}

void RenderingEngine::RenderBackgroundLayer(const BackgroundLayer& background_layer) {
  const double scroll_factor = background_layer.scroll_slowdown_factor;
  const auto& background = background_layer.background_img;

  const auto total_height_pixels =
      level_.tile_grid.GetHeight() * level_.level_tileset->GetTileSize();

  int num_of_background_draws{};
  // If the background is taller than the screensize, linearly move the camera such that you see the
  // top of the background at the top of the level, and the bottom at the bottom.
  if (background->height > kScreenHeightPx) {
    // C++ 20
    // auto y_pos = std::lerp(-background->height + kScreenHeightPx, 0.0,
    //  cam_position_px_y_ / (total_height_pixels - kScreenHeightPx));

    const double y_multiplier = static_cast<double>(background->height - kScreenHeightPx) /
                                static_cast<double>(total_height_pixels - kScreenHeightPx);
    auto y_pos =
        static_cast<int>(y_multiplier * cam_position_px_y_ - background->height + kScreenHeightPx);
    auto x_pos = -(static_cast<int>(cam_position_px_x_ / scroll_factor) % background->width);
    while (x_pos < kScreenWidthPx) {
      num_of_background_draws++;
      engine_ptr_->DrawSprite(x_pos, y_pos, background.get());
      x_pos += background->width;
    }
  } else {
    // If y is smaller than the screen size, tile both the same way.
    auto y_pos = (static_cast<int>((cam_position_px_y_ / scroll_factor) - kScreenHeightPx) %
                  background->height);
    while (y_pos < kScreenHeightPx) {
      auto x_pos = -(static_cast<int>(cam_position_px_x_ / scroll_factor) % background->width);
      while (x_pos < kScreenWidthPx) {
        num_of_background_draws++;
        engine_ptr_->DrawSprite(x_pos, y_pos, background.get());
        x_pos += background->width;
      }
      y_pos += background->height;
    }
  }
  // LOG_INFO("number of draws " << num_of_background_draws);
}

void RenderingEngine::RenderTiles() {
  KeepCameraInBounds();

  const auto position = GetCameraPosition();
  const auto& tilemap = level_.tile_grid;
  const auto& tileset = *level_.level_tileset;
  for (int y_itr = 0; y_itr <= viewport_height_ + 1; ++y_itr) {
    for (int x_itr = 0; x_itr <= viewport_width_ + 1; ++x_itr) {
      const double lookup_x = position.x + x_itr;
      const double lookup_y = position.y + y_itr;
      const int lookup_x_int = static_cast<int>(std::floor(lookup_x));
      const int lookup_y_int = static_cast<int>(std::floor(lookup_y));
      int tile_idx{};
      if (lookup_x_int >= 0 && lookup_x_int < tilemap.GetWidth() && lookup_y_int >= 0 &&
          lookup_y_int < tilemap.GetHeight()) {
        tile_idx = tilemap.GetTile(lookup_x_int, lookup_y_int).tile_id;
      }
      auto* tile = tileset.GetTile(tile_idx);
      if (tile_idx == 0) {
        continue;
      }

      const double x_fraction = lookup_x - lookup_x_int;
      const double y_fraction = lookup_y - lookup_y_int;

      const int x_px = static_cast<int>(std::round((x_itr - x_fraction) * tile_size_));
      const int y_px =
          static_cast<int>(std::round(kScreenHeightPx - (y_itr + 1 - y_fraction) * tile_size_));
      //   std::cout << " itrs " << x_itr << " " << y_itr << " "                        //
      //             << " global " << lookup_x << " " << lookup_y << " "                //
      //             << " global floor " << lookup_x_int << " " << lookup_y_int << " "  //
      //             << " faction " << x_fraction << " " << y_fraction << " "           //
      //             << " pixel " << x_px << " " << y_px << " "                         //
      //             << " tile " << tile_idx << std::endl;                              //

      engine_ptr_->DrawSprite(x_px, y_px, tile);
    }
  }
}

void RenderingEngine::RenderPlayer(Player& player) {
  const auto position_in_screen = player.position - GetCameraPosition();
  // if (position_in_screen.x < 0 || position_in_screen.y < 0 ||
  //     position_in_screen.x > viewport_width_ || position_in_screen.y > viewport_height_) {
  //   return;
  // }

  // The player position is bottom left, but the rendering engine requires top left.
  // This conversion is done here.
  const olc::Sprite* sprite = player.animation_manager.GetSprite();
  const int position_px_x = static_cast<int>(position_in_screen.x * tile_size_);
  const int position_px_y =
      kScreenHeightPx - static_cast<int>(position_in_screen.y * tile_size_) - sprite->height;
  const auto flip = player.facing_left;
  engine_ptr_->DrawSprite(position_px_x, position_px_y, const_cast<olc::Sprite*>(sprite), 1,
                          static_cast<uint8_t>(flip));

  // const auto& width = sprite->width;
  // const auto& height = sprite->height;
  // TODO:: Add a parameter to turn this on/off.
  // if (player.collisions.bottom) {
  //   engine_ptr_->DrawLine(position_px_x, position_px_y + height, position_px_x + width,
  //                         position_px_y + height);
  // }
  // if (player.collisions.top) {
  //   engine_ptr_->DrawLine(position_px_x, position_px_y, position_px_x + width, position_px_y);
  // }
  // if (player.collisions.left) {
  //   engine_ptr_->DrawLine(position_px_x, position_px_y, position_px_x, position_px_y + height);
  // }
  // if (player.collisions.right) {
  //   engine_ptr_->DrawLine(position_px_x + width, position_px_y, position_px_x + width,
  //                         position_px_y + height);
  // }
}

void RenderingEngine::KeepCameraInBounds() {
  cam_position_px_x_ = std::max(cam_position_px_x_, 0);
  cam_position_px_x_ = std::min(cam_position_px_x_, max_cam_postion_px_x_);
  cam_position_px_y_ = std::max(cam_position_px_y_, 0);
  cam_position_px_y_ = std::min(cam_position_px_y_, max_cam_postion_px_y_);
}

}  // namespace platformer