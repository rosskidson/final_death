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

  constexpr double kScreenEdgeBuffer = 0.1;
  const double max_x = grid_width - (viewport_width_ / 2) - kScreenEdgeBuffer;
  const double min_x = (viewport_width_ / 2) + 0.1;
  const double max_y = grid_height - (viewport_height_ / 2) - kScreenEdgeBuffer;
  const double min_y = (viewport_height_ / 2) + 0.1;
  camera_bounds_ = {max_x, min_x, max_y, min_y};

  position_ = {10., 10.};
}

void Camera::UpdatePosition(const Vector2d& absolute_vec) {
  position_ = absolute_vec;
  KeepCameraInBounds();
}

void Camera::MoveCamera(const Vector2d& relative_vec) {
  position_.x += relative_vec.x;
  position_.y += relative_vec.y;
  KeepCameraInBounds();
}

Vector2d Camera::GetCameraPosition() const { return position_; }

void Camera::Render() {
  KeepCameraInBounds();
  RenderTiles();
}

void Camera::RenderTiles() {
  const auto& tilemap = level_.tile_grid;
  const auto& tileset = *level_.level_tileset;
  for (int y_itr = 0; y_itr <= viewport_height_ + 1; ++y_itr) {
    for (int x_itr = 0; x_itr <= viewport_width_ + 1; ++x_itr) {
      const double lookup_x = position_.x + (x_itr - (viewport_width_ / 2.));
      const double lookup_y = position_.y + (y_itr - (viewport_height_ / 2.));
      const int lookup_x_int = static_cast<int>(std::floor(lookup_x));
      const int lookup_y_int = static_cast<int>(std::floor(lookup_y));
      int tile_idx{};
      if (lookup_x_int >= 0 && lookup_x_int < tilemap.GetWidth() && lookup_y_int >= 0 &&
          lookup_y_int < tilemap.GetHeight()) {
        tile_idx = tilemap.GetTile(lookup_x_int, lookup_y_int).tile_id;
      }
      auto* tile = tileset.GetTile(tile_idx);

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

void Camera::KeepCameraInBounds() {
  position_.x = std::max(position_.x, camera_bounds_.min_x);
  position_.x = std::min(position_.x, camera_bounds_.max_x);
  position_.y = std::max(position_.y, camera_bounds_.min_y);
  position_.y = std::min(position_.y, camera_bounds_.max_y);
}

}  // namespace platformer