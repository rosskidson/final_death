#include "platformer.h"

#include <chrono>
#include <map>
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

constexpr double kMaxVel = 2.5;
constexpr double kAcceleration = 5.0;

namespace platformer {

void Physics(Player& player) {
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

void LoadSprite(const std::string& filename,
                const std::string& name,
                std::map<std::string, olc::Sprite>& sprite_storage) {
  const auto sprite_path = std::filesystem::path(SOURCE_DIR) / "assets" / filename;
  sprite_storage[name] = olc::Sprite();
  if (sprite_storage[name].LoadFromFile(sprite_path.string()) != olc::rcode::OK) {
    std::cout << " Failed loading sprite `" << sprite_path << "`." << std::endl;
  }
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
  level_idx_ = 0;
  const auto& tile_grid = GetCurrentLevel().tile_grid;
  camera_ = std::make_unique<Camera>(this, GetCurrentLevel());

  player_.position = {10, 10};
  player_.velocity = {0, 0};

  LoadSprite("white_box_32.png", "white_box", sprite_storage_);
  LoadSprite("white_box_32_left.png", "white_box_left", sprite_storage_);
  LoadSprite("white_box_32_right.png", "white_box_right", sprite_storage_);
  LoadSprite("white_box_32_top.png", "white_box_top", sprite_storage_);
  LoadSprite("white_box_32_bottom.png", "white_box_bottom", sprite_storage_);
  player_.sprite = &sprite_storage_["white_box"];

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  this->Keyboard();
  Physics(player_);
  CollisionCheckPlayer(player_);
  camera_->KeepPlayerInFrame(player_, 0.3);
  camera_->RenderBackground();
  camera_->RenderTiles();
  camera_->RenderPlayer(player_);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return true;
}

void Platformer::CollisionCheckPlayer(Player& player) {
  const auto& collision_grid = GetCurrentLevel().property_grid;
  const int tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  constexpr double kSampleDistance = 0.2;
  // Add/Subtract an epsilon when looking up collision tiles.
  // If the location is on the border with a tile, it does not collide with it.
  constexpr double kEps = 1e-3;
  // TODO:: Change to proper hitbox with offsets
  const double collision_width = static_cast<double>(player.sprite->width) / tile_size;
  const double collision_height = static_cast<double>(player.sprite->height) / tile_size;

  std::map<Side, int> collision_counter;
  // Left
  for (double y = player.position.y - kEps; y > player.position.y - collision_height + kEps;
       y -= kSampleDistance) {
    if (collision_grid.GetTile(player.position.x, y) == 1) {
      collision_counter[Side::LEFT]++;
    }
  }
  // Right
  for (double y = player.position.y - kEps; y > player.position.y - collision_height + kEps;
       y -= kSampleDistance) {
    if (collision_grid.GetTile(player.position.x + collision_width - kEps, y) == 1) {
      collision_counter[Side::RIGHT]++;
    }
  }
  // Top
  for (double x = player.position.x + kEps; x < player.position.x + collision_width - kEps;
       x += kSampleDistance) {
    if (collision_grid.GetTile(x, player.position.y - kEps) == 1) {
      collision_counter[Side::TOP]++;
    }
  }
  // Bottom
  for (double x = player.position.x + kEps; x < player.position.x + collision_width - kEps;
       x += kSampleDistance) {
    if (collision_grid.GetTile(x, player.position.y - collision_height) == 1) {
      collision_counter[Side::BOTTOM]++;
    }
  }

  if ((collision_counter[Side::BOTTOM] > 0 && collision_counter[Side::TOP] > 0) ||
      (collision_counter[Side::LEFT] > 0 && collision_counter[Side::RIGHT] > 0)) {
    std::cout << "Squish" << std::endl;
    // exit(1);
  }

  // Get 'most colliding' side
  int max_count = 0;
  Side most_colliding_side{};
  for (const auto& [side, count] : collision_counter) {
    if (count > max_count) {
      max_count = count;
      most_colliding_side = side;
    }
  }
  if (max_count == 0) {
    player.sprite = &sprite_storage_["white_box"];
  } else if (most_colliding_side == Side::LEFT) {
    player.velocity.x = 0;
    player.position.x = std::floor(player.position.x) + 1;
    player.sprite = &sprite_storage_["white_box_left"];
  } else if (most_colliding_side == Side::RIGHT) {
    player.velocity.x = 0;
    player.position.x = std::floor(player.position.x);
    player.sprite = &sprite_storage_["white_box_right"];
  } else if (most_colliding_side == Side::TOP) {
    player.velocity.y = 0;
    player.position.y = std::floor(player.position.y);
    player.sprite = &sprite_storage_["white_box_top"];
  } else if (most_colliding_side == Side::BOTTOM) {
    player.velocity.y = 0;
    player.position.y = std::floor(player.position.y) + 1;
    player.sprite = &sprite_storage_["white_box_bottom"];
  }
}

// Check top
// if (collision_grid.GetTile(player.position.x, player.position.y) == 1 ||
//     collision_grid.GetTile(player.position.x + collision_width, player.position.y) == 1) {
//   player_.velocity.y = 0;
//   player_.position.y = static_cast<int>(player.position.y) - 0.01;
// }
// // Check bottom side.
// if (collision_grid.GetTile(player.position.x, player.position.y - collision_height) == 1 ||
//     collision_grid.GetTile(player.position.x + collision_width,
//                            player.position.y - collision_height) == 1) {
//   player_.velocity.y = 0;
//   player_.position.y = static_cast<int>(player.position.y) + 1;
// }

// Check bottom side.
// if (collision_grid.GetTile(player.position.x, player.position.y - collision_height) == 1 ||
//     collision_grid.GetTile(player.position.x + collision_width,
//                            player.position.y - collision_height) == 1) {
//   player_.velocity.y = 0;
//   player_.position.y = static_cast<int>(player.position.y) + 1;
// }

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
    player_.acceleration.x = -kAcceleration;
  } else if (this->GetKey(olc::Key::RIGHT).bHeld) {
    player_.acceleration.x = +kAcceleration;
  } else {
    player_.acceleration.x = 0.0;
  }
  if (this->GetKey(olc::Key::UP).bHeld) {
    player_.acceleration.y = +kAcceleration;
  } else if (this->GetKey(olc::Key::DOWN).bHeld) {
    player_.acceleration.y = -kAcceleration;
  } else {
    player_.acceleration.y = 0.0;
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }
}

}  // namespace platformer
