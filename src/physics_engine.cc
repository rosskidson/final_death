#include "physics_engine.h"

#include <algorithm>

#include "utils/game_clock.h"

constexpr double kMaxVelX = 8;
constexpr double kMaxVelY = 25;
constexpr double kGravity = 50.0;
constexpr double kGroundFriction = 50.0;

namespace platformer {

BoundingBox GetPlayerCollisionBox(const Player& player, int tile_size) {
  const double x_offset = static_cast<double>(player.x_offset_px) / tile_size;
  const double y_offset = static_cast<double>(player.y_offset_px) / tile_size;
  const double collision_width = static_cast<double>(player.collision_width_px) / tile_size;
  const double collision_height = static_cast<double>(player.collision_height_px) / tile_size;
  return {player.position.x + x_offset, player.position.x + x_offset + collision_width,
          player.position.y + y_offset, player.position.y + y_offset + collision_height};
}

bool IsCollision(const Grid<int>& collision_grid, double x, double y) {
  const int int_x = static_cast<int>(std::floor(x));
  const int int_y = static_cast<int>(std::floor(y));
  if (int_x < 0 || int_y < 0 || int_x >= collision_grid.GetWidth() ||
      int_y >= collision_grid.GetHeight()) {
    return false;
  }
  return collision_grid.GetTile(int_x, int_y) == 1;
}

std::pair<bool, bool> CheckPlayerAxisCollision(const Player& player,
                                               const Axis axis,
                                               const int tile_size,
                                               const Grid<int>& collision_grid) {
  constexpr double kLowerSamplePercent = 0.2;
  constexpr double kMiddlSamplePercent = 0.5;
  constexpr double kUpperSamplePercent = 0.8;
  const auto player_box = GetPlayerCollisionBox(player, tile_size);

  std::vector<Vector2d> lower_collision_points;
  std::vector<Vector2d> upper_collision_points;
  if (axis == Axis::X) {
    const auto box_height = player_box.top - player_box.bottom;
    lower_collision_points = {
        {player_box.left, player_box.bottom + box_height * kLowerSamplePercent},
        {player_box.left, player_box.bottom + box_height * kMiddlSamplePercent},
        {player_box.left, player_box.bottom + box_height * kUpperSamplePercent},
    };
    upper_collision_points = {
        {player_box.right, player_box.bottom + box_height * kLowerSamplePercent},
        {player_box.right, player_box.bottom + box_height * kMiddlSamplePercent},
        {player_box.right, player_box.bottom + box_height * kUpperSamplePercent},
    };
  } else {
    const auto box_width = player_box.right - player_box.left;
    lower_collision_points = {
        {player_box.left + box_width * kLowerSamplePercent, player_box.bottom},
        {player_box.left + box_width * kMiddlSamplePercent, player_box.bottom},
        {player_box.left + box_width * kUpperSamplePercent, player_box.bottom},
    };
    upper_collision_points = {
        {player_box.left + box_width * kLowerSamplePercent, player_box.top},
        {player_box.left + box_width * kMiddlSamplePercent, player_box.top},
        {player_box.left + box_width * kUpperSamplePercent, player_box.top},
    };
  }

  bool lower_collision{};
  bool upper_collision{};
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
  return std::make_pair(lower_collision, upper_collision);
}

void ResolveCollisions(Player& player,
                       const Axis& axis,
                       const int tile_size,
                       const bool lower_collision,
                       const bool upper_collision) {
  const auto player_box = GetPlayerCollisionBox(player, tile_size);
  constexpr double kEps = 1e-6;
  if (axis == Axis::X) {
    const auto x_offset = static_cast<double>(player.x_offset_px) / tile_size;
    const auto collision_width = static_cast<double>(player.collision_width_px) / tile_size;
    if (lower_collision) {
      // Only zero out velocity if the character is moving towards to collision
      // This prevents sticking to a platforms e.g. if it hits it on the corner when going up.
      player.velocity.x = std::max<double>(player.velocity.x, 0);
      player.collisions.left = true;
      player.position.x = std::floor(player_box.left) + 1 - x_offset;
    }
    if (upper_collision) {
      player.velocity.x = std::min<double>(player.velocity.x, 0);
      player.collisions.right = true;
      player.position.x = std::floor(player_box.right) - x_offset - collision_width - kEps;
    }
  } else {
    const auto y_offset = static_cast<double>(player.y_offset_px) / tile_size;
    const auto collision_height = static_cast<double>(player.collision_height_px) / tile_size;
    if (lower_collision) {
      player.velocity.y = std::max<double>(player.velocity.y, 0);
      player.collisions.bottom = true;
      player.position.y = std::floor(player_box.bottom) + 1 - y_offset;
    }
    if (upper_collision) {
      player.velocity.y = std::min<double>(player.velocity.y, 0);
      player.collisions.top = true;
      player.position.y = std::floor(player_box.top) - y_offset - collision_height - kEps;
    }
  }
}

PhysicsEngine::PhysicsEngine(const Level& level, std::shared_ptr<ParameterServer> parameter_server)
    : tile_size_{level.level_tileset->GetTileSize()},
      collisions_grid_{level.property_grid},
      parameter_server_{std::move(parameter_server)} {
  parameter_server_->AddParameter("physics/gravity", kGravity, "Gravity, unit is tile/s^2");
  parameter_server_->AddParameter("physics/max.x.vel", kMaxVelX,
                                  "Maximum horizontal velocity of the player");
  parameter_server_->AddParameter("physics/max.y.vel", kMaxVelY,
                                  "Maximum vertical velocity of the player");
  parameter_server_->AddParameter("physics/friction", kGroundFriction,
                                  "Controls deceleration on the ground.");
}

void PhysicsEngine::PhysicsStep(Player& player) {
  const auto now = GameClock::NowGlobal();
  const double delta_t = (now - player.last_update).count() / 1e9;

  player.acceleration.y = -1 * parameter_server_->GetParameter<double>("physics/gravity");

  const auto max_x_vel = parameter_server_->GetParameter<double>("physics/max.x.vel");
  const auto max_y_vel = parameter_server_->GetParameter<double>("physics/max.y.vel");
  const auto friction = parameter_server_->GetParameter<double>("physics/friction");

  if (player.collisions.bottom && player.acceleration.x == 0) {
    if (std::abs(player.velocity.x) < friction * delta_t) {
      player.velocity.x = 0;
    } else {
      player.velocity.x -= friction * delta_t * (player.velocity.x > 0 ? 1 : -1);
    }
  }

  player.collisions = {};

  player.velocity.x += player.acceleration.x * delta_t;
  player.velocity.x = std::min(player.velocity.x, max_x_vel);
  player.velocity.x = std::max(player.velocity.x, -max_x_vel);
  player.position.x += player.velocity.x * delta_t;
  this->CheckPlayerCollision(player, Axis::X);

  player.velocity.y += player.acceleration.y * delta_t;
  player.velocity.y = std::min(player.velocity.y, max_y_vel);
  player.velocity.y = std::max(player.velocity.y, -max_y_vel);
  player.position.y += player.velocity.y * delta_t;
  this->CheckPlayerCollision(player, Axis::Y);

  if (player.velocity.x != 0.) {
    player.facing_left = player.velocity.x < 0;
  }

  player.last_update = now;
}

void PhysicsEngine::CheckPlayerCollision(Player& player, const Axis& axis) {
  const auto [lower_collision, upper_collision] =
      CheckPlayerAxisCollision(player, axis, tile_size_, collisions_grid_);

  if (!lower_collision && !upper_collision) {
    return;
  }
  ResolveCollisions(player, axis, tile_size_, lower_collision, upper_collision);
}

}  // namespace platformer