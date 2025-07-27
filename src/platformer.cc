#include "platformer.h"

#include <chrono>
#include <thread>

#include "game_configuration.h"
#include "load_game_configuration.h"
#include "utils/logging.h"

#include "config.h"

namespace platformer {

constexpr double kTileSize = 16.;
constexpr int kScreenWidthPx = 300;
constexpr int kScreenHeightPx = 300;

Platformer::Platformer() { this->Construct(kScreenWidthPx, kScreenHeightPx, 4, 4); }

void Platformer::Render(const double x, const double y) {
  const int viewport_width = kScreenWidthPx / kTileSize;
  const int viewport_height = kScreenHeightPx / kTileSize;
  const auto& tilemap = config_.levels.front().tile_grid;
  const auto& tileset = *config_.levels.front().level_tileset;
  for (int y_itr = 0; y_itr <= viewport_height + 1; ++y_itr) {
    for (int x_itr = 0; x_itr <= viewport_width + 1; ++x_itr) {
      const double lookup_x = x + (x_itr - (viewport_width / 2.));
      const double lookup_y = y + (y_itr - (viewport_height / 2.));
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

      const int x_px = std::round((x_itr - x_fraction) * kTileSize);
      const int y_px = std::round(kScreenHeightPx - (y_itr + 1 - y_fraction) * kTileSize);
      // std::cout << " itrs " << x_itr << " " << y_itr << " "                        //
      //           << " global " << lookup_x << " " << lookup_y << " "                //
      //           << " global floor " << lookup_x_int << " " << lookup_y_int << " "  //
      //           << " faction " << x_fraction << " " << y_fraction << " "           //
      //           << " pixel " << x_px << " " << y_px << " "                         //
      //           << " tile " << tile_idx << std::endl;                              //

      this->DrawSprite(x_px, y_px, tile);
    }
  }
}

bool Platformer::OnUserCreate() {
  // TODO(FOR RELEASE): Path is assumed to be cmake source. Store it in the binary, or do a proper install
  const auto levels_path = std::filesystem::path(SOURCE_DIR) / "levels.json";
  auto config = platformer::LoadGameConfiguration(levels_path.string());
  if (!config.has_value()) {
    std::cout << "Failed loading config " << std::endl;
    return false;
  }
  config_ = std::move(*config);

  const auto& int_grid = config_.levels.front().tile_grid;
  for (int y = int_grid.GetHeight() - 1; y >= 0; --y) {
    for (int x = 0; x < int_grid.GetWidth(); ++x) {
      std::cout << int_grid.GetTile(x, y).tile_id << " ";
    }
    std::cout << std::endl;
  }
  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  // this->DrawSprite(-10, 10, config_.levels.front().level_tileset->GetTile(691));
  static double x = 10;
  static double y = 10;
  Render(x, y);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (this->GetKey(olc::Key::RIGHT).bHeld) {
    x += 0.0625;
  }
  if (this->GetKey(olc::Key::LEFT).bHeld) {
    x -= 0.0625;
  }
  if (this->GetKey(olc::Key::UP).bHeld) {
    y += 0.0625;
  }
  if (this->GetKey(olc::Key::DOWN).bHeld) {
    y -= 0.0625;
  }
  const double viewport_width = kScreenWidthPx / kTileSize;
  const double viewport_height = kScreenHeightPx / kTileSize;
  const double max_x = config_.levels.front().tile_grid.GetWidth() - (viewport_width / 2) - 0.1;
  const double min_x = (viewport_width / 2) + 0.1;
  const double max_y = config_.levels.front().tile_grid.GetHeight() - (viewport_height / 2) - 0.1;
  const double min_y = (viewport_height / 2) + 0.1;
  x = std::min(x, max_x);
  x = std::max(x, min_x);
  y = std::min(y, max_y);
  y = std::max(y, min_y);

  return true;
}

}  // namespace platformer
