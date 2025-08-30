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

BoundingBox GetPlayerCollisionBox(const Player& player, int tile_size) {
  const double x_offset = static_cast<double>(player.x_offset_px) / tile_size;
  const double y_offset = static_cast<double>(player.y_offset_px) / tile_size;
  const double collision_width = static_cast<double>(player.collision_width_px) / tile_size;
  const double collision_height = static_cast<double>(player.collision_height_px) / tile_size;
  return {player.position.x + x_offset, player.position.x + x_offset + collision_width,
          player.position.y + y_offset, player.position.y + y_offset + collision_height};
}

bool IsCollision(const Grid<int>& collision_grid, int x, int y) {
  if (x < 0 || y < 0 || x >= collision_grid.GetWidth() || y >= collision_grid.GetHeight()) {
    return false;
  }
  return collision_grid.GetTile(x, y) == 1;
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
  player_.x_offset_px = 7;
  player_.y_offset_px = 0;
  player_.collision_width_px = 12;
  player_.collision_height_px = 22;

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  this->Keyboard();
  Physics(player_);
  // CollisionCheckPlayer(player_);
  camera_->KeepPlayerInFrame(player_, 0.3);
  camera_->RenderBackground();
  camera_->RenderTiles();
  camera_->RenderPlayer(player_);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return true;
}

void Platformer::Physics(Player& player) {
  auto now = Clock::now();
  const double delta_t = (now - player.last_update).count() / 1e9;

  player.collides_bottom = false;
  player.collides_top = false;
  player.collides_left = false;
  player.collides_right = false;

  player.acceleration.y = -kGravity;

  player.velocity.x += player.acceleration.x * delta_t;
  player.velocity.x = std::min(player.velocity.x, kMaxVelX);
  player.velocity.x = std::max(player.velocity.x, -kMaxVelX);
  player.position.x += player.velocity.x * delta_t;
  this->CheckPlayerCollision(player, Axis::X);

  player.velocity.y += player.acceleration.y * delta_t;
  player.velocity.y = std::min(player.velocity.y, kMaxVelY);
  player.velocity.y = std::max(player.velocity.y, -kMaxVelY);
  player.position.y += player.velocity.y * delta_t;
  this->CheckPlayerCollision(player, Axis::Y);

  if (player.velocity.x != 0.) {
    player.facing_left = player.velocity.x < 0;
  }

  player.last_update = now;
}

void Platformer::CheckPlayerCollision(Player& player, const Axis& axis) {
  const int tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  const auto player_box = GetPlayerCollisionBox(player, tile_size);

  std::vector<Vector2d> lower_collision_points;
  std::vector<Vector2d> upper_collision_points;
  if (axis == Axis::X) {
    const auto box_height = player_box.top - player_box.bottom;
    lower_collision_points = {
        {player_box.left, player_box.bottom + box_height * 0.25},
        {player_box.left, player_box.bottom + box_height * 0.5},
        {player_box.left, player_box.bottom + box_height * 0.75},
    };
    upper_collision_points = {
        {player_box.right, player_box.bottom + box_height * 0.25},
        {player_box.right, player_box.bottom + box_height * 0.5},
        {player_box.right, player_box.bottom + box_height * 0.75},
    };
  } else {
    const auto box_width = player_box.right - player_box.left;
    lower_collision_points = {
        {player_box.left + box_width * 0.25, player_box.bottom},
        {player_box.left + box_width * 0.5, player_box.bottom},
        {player_box.left + box_width * 0.75, player_box.bottom},
    };
    upper_collision_points = {
        {player_box.left + box_width * 0.25, player_box.top},
        {player_box.left + box_width * 0.5, player_box.top},
        {player_box.left + box_width * 0.75, player_box.top},
    };
  }

  bool lower_collision{};
  bool upper_collision{};
  const auto& collision_grid = GetCurrentLevel().property_grid;
  for (const auto& pt : lower_collision_points) {
    lower_collision |= IsCollision(collision_grid, pt.x, pt.y);
  }
  for (const auto& pt : upper_collision_points) {
    upper_collision |= IsCollision(collision_grid, pt.x, pt.y);
  }
  if (lower_collision && upper_collision) {
    const std::string axis_str = axis == Axis::X ? "Horizontal" : "Vertical";
    std::cout << axis_str << " Squish!" << std::endl;
  }

  if (!lower_collision && !upper_collision) {
    return;
  }
  constexpr double kEps = 1e-6;
  if (axis == Axis::X) {
    const auto x_offset = static_cast<double>(player.x_offset_px) / tile_size;
    const auto collision_width = static_cast<double>(player.collision_width_px) / tile_size;
    if (lower_collision) {
      // Only zero out velocity if the character is moving towards to collision
      // This prevents sticking to a platforms e.g. if it hits it on the corner when going up.
      if (player.velocity.x < 0) {
        player.velocity.x = 0;
      }
      player.collides_left = true;
      player.position.x = std::floor(player_box.left) + 1 - x_offset;
    }
    if (upper_collision) {
      if (player.velocity.x > 0) {
        player.velocity.x = 0;
      }
      player.collides_right = true;
      player.position.x = std::floor(player_box.right) - x_offset - collision_width - kEps;
    }
  } else {
    const auto y_offset = static_cast<double>(player.y_offset_px) / tile_size;
    const auto collision_height = static_cast<double>(player.collision_height_px) / tile_size;
    if (lower_collision) {
      if (player.velocity.y < 0) {
        player.velocity.y = 0;
      }
      player.collides_bottom = true;
      player.position.y = std::floor(player_box.bottom) + 1 - y_offset;
    }
    if (upper_collision) {
      if (player.velocity.y > 0) {
        player.velocity.y = 0;
      }
      player.collides_top = true;
      player.position.y = std::floor(player_box.top) - y_offset - collision_height - kEps;
    }
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
  }

  // if (this->GetKey(olc::Key::UP).bHeld) {
  //   player_.acceleration.y = kAcceleration;
  // } else if (this->GetKey(olc::Key::DOWN).bHeld) {
  //   player_.acceleration.y = -kAcceleration;
  // } else {
  //   player_.acceleration.y = 0;
  // }

  if (this->GetKey(olc::Key::SPACE).bPressed) {
    player_.velocity.y = kJumpVel;
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }
}

}  // namespace platformer
