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

constexpr int kPixelSize = 3;

constexpr double kMaxVelX = 10;
constexpr double kMaxVelY = 25;
constexpr double kAcceleration = 50.0;
constexpr double kGravity = 30.0;
constexpr double kJumpVel = 17.0;

namespace platformer {

void Physics(Player& player) {
  auto now = Clock::now();
  const double delta_t = (now - player.last_update).count() / 1e9;

  player.acceleration.y = -kGravity;

  player.velocity.x += player.acceleration.x * delta_t;
  player.velocity.y += player.acceleration.y * delta_t;

  player.velocity.x = std::min(player.velocity.x, kMaxVelX);
  player.velocity.x = std::max(player.velocity.x, -kMaxVelX);
  player.velocity.y = std::min(player.velocity.y, kMaxVelY);
  player.velocity.y = std::max(player.velocity.y, -kMaxVelY);

  player.position.x += player.velocity.x * delta_t;
  player.position.y += player.velocity.y * delta_t;

  if (player.velocity.x != 0.) {
    player.facing_left = player.velocity.x < 0;
  }

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

  LoadSprite("player/idle.png", "bot_idle", sprite_storage_);
  LoadSprite("player/jump.png", "bot_jump", sprite_storage_);
  LoadSprite("player/run1.png", "bot_run0", sprite_storage_);
  LoadSprite("player/run2.png", "bot_run1", sprite_storage_);
  LoadSprite("player/run3.png", "bot_run2", sprite_storage_);
  LoadSprite("player/run4.png", "bot_run3", sprite_storage_);
  player_.sprite = &sprite_storage_["bot_idle"];

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

bool IsCollision(const Grid<int>& collision_grid, int x, int y) {
  if (x < 0 || y < 0 || x >= collision_grid.GetWidth() || y >= collision_grid.GetHeight()) {
    return false;
  }
  return collision_grid.GetTile(x, y) == 1;
}

bool Platformer::PlayerCollidesWithMap(Player& player) {
  const auto& collision_grid = GetCurrentLevel().property_grid;
  const int tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  constexpr double kSampleDistance = 0.2;
  // Add/Subtract an epsilon when looking up collision tiles.
  // If the location is on the border with a tile, it does not collide with it.
  const double x_offset = 6. / tile_size;
  const double y_offset = 5. / tile_size;
  const double collision_width = 15. / tile_size;
  const double collision_height = 23. / tile_size;

  const double player_x = player.position.x + x_offset;
  const double player_y = player.position.y - y_offset;
  std::map<Side, int> collision_counter;
  for (double y = player_y - kSampleDistance / 2; y > player_y - collision_height;
       y -= kSampleDistance) {
    // Left
    if (IsCollision(collision_grid, player_x, y)) {
      collision_counter[Side::LEFT]++;
    }
    // Right
    if (IsCollision(collision_grid, player_x + collision_width, y)) {
      collision_counter[Side::RIGHT]++;
    }
  }
  for (double x = player_x + kSampleDistance / 2; x < player_x + collision_width;
       x += kSampleDistance) {
    // Top
    if (IsCollision(collision_grid, x, player_y)) {
      collision_counter[Side::TOP]++;
    }
    // Bottom
    if (IsCollision(collision_grid, x, player_y - collision_height)) {
      collision_counter[Side::BOTTOM]++;
    }
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
    return false;
  }

  constexpr double kEps = 1e-3;
  if (most_colliding_side == Side::LEFT) {
    player.collides_left = true;
    if (player.velocity.x < 0) {
      player.velocity.x = 0;
    }
    player.position.x = std::floor(player.position.x) + 1 - x_offset;
    return true;
  }
  if (most_colliding_side == Side::RIGHT) {
    player.collides_right = true;
    if (player.velocity.x > 0) {
      player.velocity.x = 0;
    }
    player.position.x = std::floor(player_x + collision_width) - x_offset - collision_width - kEps;
    return true;
  }
  if (most_colliding_side == Side::TOP) {
    player.collides_top = true;
    if (player.velocity.y > 0) {
      player.velocity.y = 0;
    }
    player.position.y = std::floor(player_y) + y_offset - kEps;
    return true;
  }
  if (most_colliding_side == Side::BOTTOM) {
    player.collides_bottom = true;
    if (player.velocity.y < 0) {
      player.velocity.y = 0;
    }
    player.position.y = std::floor(player_y - collision_height) + 1 + collision_height + y_offset;
    return true;
  }
  return false;
}

void Platformer::CollisionCheckPlayer(Player& player) {
  player.collides_bottom = false;
  player.collides_top = false;
  player.collides_left = false;
  player.collides_right = false;
  // There should be maximum of two adjustments needed: x and y.
  for (int i = 0; i < 2; ++i) {
    if (!PlayerCollidesWithMap(player)) {
      return;
    }
  }
  // If there is still a collision, this probably means the player is being squished.
  if (PlayerCollidesWithMap(player)) {
    std::cout << "Squish!" << std::endl;
  }
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

  auto now = Clock::now();
  if (this->GetKey(olc::Key::LEFT).bPressed) {
    player_.sprite = &sprite_storage_["bot_run1"];
    player_.animation_update = now;
  }
  if (this->GetKey(olc::Key::RIGHT).bPressed) {
    player_.sprite = &sprite_storage_["bot_run1"];
    player_.animation_update = now;
  }

  if (this->GetKey(olc::Key::LEFT).bHeld) {
    player_.acceleration.x = -kAcceleration;
    if (((now - player_.animation_update).count() / 1e9) > 0.1) {
      player_.animation_frame++;
      const std::string next_frame = "bot_run" + std::to_string(player_.animation_frame % 4);
      // std::cout << next_frame << std::endl;
      player_.sprite = &sprite_storage_[next_frame];
      player_.animation_update = now;
    }
  } else if (this->GetKey(olc::Key::RIGHT).bHeld) {
    player_.acceleration.x = +kAcceleration;
    if (((now - player_.animation_update).count() / 1e9) > 0.1) {
      player_.animation_frame++;
      const std::string next_frame = "bot_run" + std::to_string(player_.animation_frame % 4);
      // std::cout << next_frame << std::endl;
      player_.sprite = &sprite_storage_[next_frame];
      player_.animation_update = now;
    }
  } else {
    player_.sprite = &sprite_storage_["bot_idle"];
    player_.acceleration.x = -10 * player_.velocity.x;
    // player_.velocity.x = 0.0;
  }

  if (this->GetKey(olc::Key::SPACE).bPressed) {
    player_.velocity.y = kJumpVel;
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }
}

}  // namespace platformer
