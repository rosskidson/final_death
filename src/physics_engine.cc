#include "physics_engine.h"

#include <algorithm>

#include "player.h"
#include "player_state.h"
#include "utils/game_clock.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

constexpr double kMaxVelX = 8;
constexpr double kMaxVelY = 25;
constexpr double kRollVelX = 15;
constexpr double kSlideVelX = 10;
constexpr double kGravity = 50.0;
constexpr double kGroundFriction = 50.0;
constexpr double kAirFriction = 1.0;
constexpr double kSlideFriction = 5.0;  // For backdodge

namespace platformer {

void UpdateCollisionsChanged(Collisions& collisions, const Collisions& old_collisions) {
  collisions.left_changed = collisions.left != old_collisions.left;
  collisions.right_changed = collisions.right != old_collisions.right;
  collisions.top_changed = collisions.top != old_collisions.top;
  collisions.bottom_changed = collisions.bottom != old_collisions.bottom;
}

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

std::pair<double, double> GetMaxVelocity(const Player& player,
                                         const ParameterServer& parameter_server) {
  if (player.state == PlayerState::BackDodgeShot) {
    return {parameter_server.GetParameter<double>("physics/slide.x.vel"),
            parameter_server.GetParameter<double>("physics/max.y.vel")};
  }
  if (player.state == PlayerState::Roll) {
    return {parameter_server.GetParameter<double>("physics/roll.x.vel"),
            parameter_server.GetParameter<double>("physics/max.y.vel")};
  }
  return {parameter_server.GetParameter<double>("physics/max.x.vel"),
          parameter_server.GetParameter<double>("physics/max.y.vel")};
}

AxisCollisions PhysicsEngine::CheckPlayerAxisCollision(const Player& player,
                                                       const Axis axis) const {
  constexpr double kLowerSamplePercent = 0.2;
  constexpr double kMiddlSamplePercent = 0.5;
  constexpr double kUpperSamplePercent = 0.8;
  const auto player_box = GetPlayerCollisionBox(player, tile_size_);

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
    lower_collision |= IsCollision(collisions_grid_, pt.x, pt.y);
  }
  for (const auto& pt : upper_collision_points) {
    upper_collision |= IsCollision(collisions_grid_, pt.x, pt.y);
  }
  return {lower_collision, upper_collision};
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

void ApplyFriction(const ParameterServer& parameter_server, const double delta_t, Player& player) {
  if (player.acceleration.x != 0) {
    return;
  }
  if (!player.collisions.bottom) {
    // Air drag: resistance is proportional to velocity.
    const auto air_friction = parameter_server.GetParameter<double>("physics/air.friction");
    player.velocity.x -= player.velocity.x * air_friction * delta_t;
    return;
  }
  const std::string ground_friction_key = player.state == PlayerState::BackDodgeShot
                                              ? "physics/slide.friction"
                                              : "physics/ground.friction";
  const auto ground_friction = parameter_server.GetParameter<double>(ground_friction_key);
  if (std::abs(player.velocity.x) < ground_friction * delta_t) {
    player.velocity.x = 0;
    return;
  }
  // Coulomb friction: Resistance is relative to normal force, independent of velocity.
  player.velocity.x -= ground_friction * delta_t * (player.velocity.x > 0 ? 1 : -1);
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
  parameter_server_->AddParameter("physics/roll.x.vel", kRollVelX,
                                  "Maximum horizontal velocity of the player during a roll");
  parameter_server_->AddParameter("physics/slide.x.vel", kSlideVelX,
                                  "Maximum horizontal velocity of the player during a slide");
  parameter_server_->AddParameter("physics/ground.friction", kGroundFriction,
                                  "Controls deceleration on the ground.");
  parameter_server_->AddParameter("physics/air.friction", kAirFriction,
                                  "Controls deceleration in the air.");
  parameter_server_->AddParameter("physics/slide.friction", kSlideFriction,
                                  "Controls deceleration in the air.");
}

void PhysicsEngine::PhysicsStep(const double delta_t, Player& player) {
  player.acceleration.y = -1 * parameter_server_->GetParameter<double>("physics/gravity");

  ApplyFriction(*parameter_server_, delta_t, player);

  player.collisions = {};

  const auto [max_x_vel, max_y_vel] = GetMaxVelocity(player, *parameter_server_);

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

  if (player.velocity.x != 0. && player.state != PlayerState::BackDodgeShot) {
    player.facing_left = player.velocity.x < 0;
  }

  if (player.velocity.y < 0) {
    player.distance_fallen += -1 * player.velocity.y * delta_t;
  }
}

void PhysicsEngine::PhysicsStep(Player& player) {
  const auto now = GameClock::NowGlobal();
  const double delta_t = (now - player.last_update).count() / 1e9;
  Collisions old_collisions = player.collisions;

  if (delta_t > 0.1) {
    // Collision detection will not work if the game is running very slow (<10hz).
    // Therefore break it up into many smaller steps.
    const int num_steps = static_cast<int>(delta_t / 0.02);
    const double delta_t_fraction = delta_t / num_steps;
    for (int i = 0; i < num_steps; ++i) {
      PhysicsStep(delta_t_fraction, player);
    }
  } else {
    PhysicsStep(delta_t, player);
  }

  UpdateCollisionsChanged(player.collisions, old_collisions);

  player.last_update = now;
}

void PhysicsEngine::CheckPlayerCollision(Player& player, const Axis& axis) const {
  const auto [lower_collision, upper_collision] = CheckPlayerAxisCollision(player, axis);

  if (lower_collision && upper_collision) {
    const std::string axis_str = axis == Axis::X ? "Horizontal" : "Vertical";
    LOG_ERROR(axis_str << " Squish!");
  }

  if (!lower_collision && !upper_collision) {
    return;
  }
  ResolveCollisions(player, axis, tile_size_, lower_collision, upper_collision);
}

}  // namespace platformer