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
using Clock = std::chrono::high_resolution_clock;

constexpr int kPixelSize = 2;

namespace platformer {

void Physics(Player& player) {
  constexpr double kMaxVel = 2.5;
  auto now = Clock::now();
  const double delta_t = (now - player.last_update).count() / 1e9;

  player.velocity.x += player.acceleration.x * delta_t;
  player.velocity.y += player.acceleration.y * delta_t;

  player.velocity.x = std::min(player.velocity.x, kMaxVel);
  player.velocity.x = std::max(player.velocity.x, -kMaxVel);
  player.velocity.y = std::min(player.velocity.y, kMaxVel);
  player.velocity.y = std::max(player.velocity.y, -kMaxVel);

  player.position.x += player.velocity.x * delta_t;
  player.position.y += player.velocity.y * delta_t;

  player.last_update = now;
}

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

  const auto sprite_path = std::filesystem::path(SOURCE_DIR) / "assets" / "white_box_32.png";
  sprite_storage_["white_box"] = olc::Sprite();
  if (sprite_storage_["white_box"].LoadFromFile(sprite_path.string()) != olc::rcode::OK) {
    std::cout << " Failed loading sprite `" << sprite_path << "`." << std::endl;
  }
  player_.sprite = &sprite_storage_["white_box"];

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  this->Keyboard();
  Physics(player_);
  camera_->RenderBackground();
  camera_->RenderTiles();
  camera_->RenderPlayer(player_);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return true;
}

void Platformer::Keyboard() {
  Vector2d pos{};
  const auto tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  constexpr double kMoveOffset = 0.03;
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
    player_.acceleration.x = -3.0;
  }
  if (this->GetKey(olc::Key::RIGHT).bHeld) {
    player_.acceleration.x = +3.0;
  }
  if (this->GetKey(olc::Key::UP).bHeld) {
    player_.acceleration.y = +3.0;
  }
  if (this->GetKey(olc::Key::DOWN).bHeld) {
    player_.acceleration.y = -3.0;
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }
}

}  // namespace platformer
