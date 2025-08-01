#include "platformer.h"

#include <chrono>
#include <memory>
#include <thread>

#include "camera.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "load_game_configuration.h"
#include "utils/logging.h"

namespace platformer {

Platformer::Platformer() { this->Construct(kScreenWidthPx, kScreenHeightPx, 2, 2); }

bool Platformer::OnUserCreate() {
  // TODO(FOR RELEASE): Path is assumed to be cmake source. Store it in the binary, or do a proper
  // install
  const auto levels_path = std::filesystem::path(SOURCE_DIR) / "levels.json";
  auto config = platformer::LoadGameConfiguration(levels_path.string());
  if (!config.has_value()) {
    std::cout << "Failed loading config " << std::endl;
    return false;
  }
  config_ = std::move(*config);
  level_ = 0;
  const auto& tile_grid = GetCurrentLevel().tile_grid;
  camera_ = std::make_unique<Camera>(this, GetCurrentLevel());

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  camera_->Render();
  this->ManuallyMoveCamera();

  return true;
}

void Platformer::ManuallyMoveCamera() {
  Vector2d pos{};
  const auto tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  if (this->GetKey(olc::Key::A).bHeld) {
    pos.x -= 1. / tile_size;
  }
  if (this->GetKey(olc::Key::D).bHeld) {
    pos.x += 1. / tile_size;
  }
  if (this->GetKey(olc::Key::W).bHeld) {
    pos.y += 1. / tile_size;
  }
  if (this->GetKey(olc::Key::S).bHeld) {
    pos.y -= 1. / tile_size;
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }

  camera_->MoveCamera(pos);
}

}  // namespace platformer
