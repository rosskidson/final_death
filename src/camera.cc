#include "camera.h"

#include <algorithm>

#include "basic_types.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "tileset.h"

namespace platformer {

Camera::Camera(olc::PixelGameEngine* engine_ptr, Level level)
    : engine_ptr_{engine_ptr}, level_{std::move(level)} {
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

void Camera::SetCameraPosition(const Vector2d& absolute_vec) {
  cam_position_px_x_ = absolute_vec.x * tile_size_;
  cam_position_px_y_ = absolute_vec.y * tile_size_;
  KeepCameraInBounds();
}

void Camera::MoveCamera(const Vector2d& relative_vec) {
  cam_position_px_x_ += relative_vec.x * tile_size_;
  cam_position_px_y_ += relative_vec.y * tile_size_;
  KeepCameraInBounds();
}

Vector2d Camera::GetCameraPosition() const {
  return Vector2d{static_cast<double>(cam_position_px_x_) / tile_size_,
                  static_cast<double>(cam_position_px_y_) / tile_size_};
}

void Camera::KeepPlayerInFrame(const Player& player, double screen_ratio) {
  const auto position = GetCameraPosition();
  const auto convert_to_px = [&](double val) -> int { return val * tile_size_; };
  const int x_max_px = convert_to_px(player.position.x - viewport_width_ * screen_ratio);
  const int x_min_px = convert_to_px(player.position.x - viewport_width_ * (1 - screen_ratio));
  const int y_max_px = convert_to_px(player.position.y - viewport_height_ * screen_ratio);
  const int y_min_px = convert_to_px(player.position.y - viewport_height_ * (1 - screen_ratio));
  cam_position_px_x_ = std::max(cam_position_px_x_, x_min_px);
  cam_position_px_x_ = std::min(cam_position_px_x_, x_max_px);
  cam_position_px_y_ = std::max(cam_position_px_y_, y_min_px);
  cam_position_px_y_ = std::min(cam_position_px_y_, y_max_px);
}

void Camera::RenderBackground() {
  for (int y = 0; y < kScreenHeightPx; ++y) {
    for (int x = 0; x < kScreenWidthPx; ++x) {
      engine_ptr_->Draw(x, y, olc::BLACK);
    }
  }
}

void Camera::RenderTiles() {
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

      const int x_px = std::round((x_itr - x_fraction) * tile_size_);
      const int y_px = std::round(kScreenHeightPx - (y_itr + 1 - y_fraction) * tile_size_);
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

void Camera::RenderPlayer(const Player& player) {
  const auto position_in_screen = player.position - GetCameraPosition();
  if (position_in_screen.x < 0 || position_in_screen.y < 0 ||
      position_in_screen.x > viewport_width_ || position_in_screen.y > viewport_height_) {
    return;
  }
  int position_px_x = static_cast<int>(position_in_screen.x * tile_size_);
  int position_px_y = kScreenHeightPx - static_cast<int>(position_in_screen.y * tile_size_);
  engine_ptr_->DrawSprite(position_px_x, position_px_y, player.sprite);
}

void Camera::KeepCameraInBounds() {
  cam_position_px_x_ = std::max(cam_position_px_x_, 0);
  cam_position_px_x_ = std::min(cam_position_px_x_, max_cam_postion_px_x_);
  cam_position_px_y_ = std::max(cam_position_px_y_, 0);
  cam_position_px_y_ = std::min(cam_position_px_y_, max_cam_postion_px_y_);
}

}  // namespace platformer