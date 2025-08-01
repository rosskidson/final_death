#include "platformer.h"

#include <chrono>
#include <memory>
#include <thread>

#include "basic_types.h"
#include "camera.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "load_game_configuration.h"
#include "utils/logging.h"

constexpr int kPixelSize = 8;

namespace platformer {

Platformer::Platformer() {
  this->Construct(kScreenWidthPx, kScreenHeightPx, kPixelSize, kPixelSize);
}

bool Platformer::OnUserCreate() {
  // TODO(FOR RELEASE): Path is assumed to be cmake source. Store it in the binary, or do a proper
  // install
  this->SetPixelMode(olc::Pixel::Mode::MASK);
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

  player_.position = {10, 10};
  player_.velocity = {0, 0};
  // const auto sprite_path = std::filesystem::path(SOURCE_DIR) / "white_box_32.png";
  // player_.sprite = olc::Sprite(sprite_path.string());
  player_.sprite_width_px = 32;
  player_.sprite_height_px = 32;

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  camera_->RenderBackground();
  camera_->RenderTiles();
  camera_->RenderPlayer(player_);
  this->Keyboard();

  // std::cout << player_.position.x << " " << player_.position.y << std::endl;

  return true;
}

void Platformer::Keyboard() {
  Vector2d pos{};
  const auto tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  constexpr double kMoveOffset = 0.004;
  const double kCamMoveOffset = 1. / tile_size;
  if (this->GetKey(olc::Key::A).bHeld) {
    pos.x -= kCamMoveOffset;
  }
  if (this->GetKey(olc::Key::D).bHeld) {
    pos.x += kCamMoveOffset;
  }
  if (this->GetKey(olc::Key::W).bHeld) {
    pos.y += kCamMoveOffset;
  }
  if (this->GetKey(olc::Key::S).bHeld) {
    pos.y -= kCamMoveOffset;
  }
  camera_->MoveCamera(pos);

  if (this->GetKey(olc::Key::LEFT).bHeld) {
    player_.position.x -= kMoveOffset;
  }
  if (this->GetKey(olc::Key::RIGHT).bHeld) {
    player_.position.x += kMoveOffset;
  }
  if (this->GetKey(olc::Key::UP).bHeld) {
    player_.position.y += kMoveOffset;
  }
  if (this->GetKey(olc::Key::DOWN).bHeld) {
    player_.position.y -= kMoveOffset;
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }
}

}  // namespace platformer
