#include "physics_system.h"

#include <algorithm>

#include "common_types/actor_state.h"
#include "common_types/basic_types.h"
#include "common_types/components.h"
#include "registry.h"
#include "registry_helpers.h"
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
constexpr double kSlideFriction = 7.0;  // For backdodge

namespace platformer {

void UpdateCollisionsChanged(Collision& collisions, const Collision& old_collisions) {
  collisions.left_changed = collisions.left != old_collisions.left;
  collisions.right_changed = collisions.right != old_collisions.right;
  collisions.top_changed = collisions.top != old_collisions.top;
  collisions.bottom_changed = collisions.bottom != old_collisions.bottom;
}

BoundingBox GetCollisionBoxInGlobalCoordinates(const Position& position,
                                               const CollisionBox& collision_box,
                                               int tile_size) {
  const double x_offset = static_cast<double>(collision_box.x_offset_px) / tile_size;
  const double y_offset = static_cast<double>(collision_box.y_offset_px) / tile_size;
  const double collision_width = static_cast<double>(collision_box.collision_width_px) / tile_size;
  const double collision_height =
      static_cast<double>(collision_box.collision_height_px) / tile_size;
  return {position.x + x_offset, position.x + x_offset + collision_width, position.y + y_offset,
          position.y + y_offset + collision_height};
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

AxisCollisions PhysicsSystem::CheckAxisCollision(const Position& position,
                                                 const CollisionBox& bounding_box,
                                                 const Axis axis) const {
  constexpr double kLowerSamplePercent = 0.2;
  constexpr double kMiddlSamplePercent = 0.5;
  constexpr double kUpperSamplePercent = 0.8;
  const auto player_box = GetCollisionBoxInGlobalCoordinates(position, bounding_box, tile_size_);

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

void PhysicsSystem::ResolveCollisions(EntityId id,
                                      const Axis& axis,
                                      const int tile_size,
                                      const bool lower_collision,
                                      const bool upper_collision) {
  auto [position, velocity, collision_box, collisions] =
      registry_->GetComponents<Position, Velocity, CollisionBox, Collision>(id);
  const auto player_box = GetCollisionBoxInGlobalCoordinates(position, collision_box, tile_size);
  constexpr double kEps = 1e-6;
  if (axis == Axis::X) {
    const auto x_offset = static_cast<double>(collision_box.x_offset_px) / tile_size;
    const auto collision_width = static_cast<double>(collision_box.collision_width_px) / tile_size;
    if (lower_collision) {
      // Only zero out velocity if the character is moving towards to collision
      // This prevents sticking to a platforms e.g. if it hits it on the corner when going up.
      velocity.x = std::max<double>(velocity.x, 0);
      collisions.left = true;
      position.x = std::floor(player_box.left) + 1 - x_offset;
    }
    if (upper_collision) {
      velocity.x = std::min<double>(velocity.x, 0);
      collisions.right = true;
      position.x = std::floor(player_box.right) - x_offset - collision_width - kEps;
    }
  } else {
    const auto y_offset = static_cast<double>(collision_box.y_offset_px) / tile_size;
    const auto collision_height =
        static_cast<double>(collision_box.collision_height_px) / tile_size;
    if (lower_collision) {
      velocity.y = std::max<double>(velocity.y, 0);
      collisions.bottom = true;
      position.y = std::floor(player_box.bottom) + 1 - y_offset;
    }
    if (upper_collision) {
      velocity.y = std::min<double>(velocity.y, 0);
      collisions.top = true;
      position.y = std::floor(player_box.top) - y_offset - collision_height - kEps;
    }
  }
}

PhysicsSystem::PhysicsSystem(const Level& level,
                             std::shared_ptr<ParameterServer> parameter_server,
                             std::shared_ptr<Registry> registry)
    : tile_size_{level.level_tileset->GetTileSize()},
      collisions_grid_{level.property_grid},
      parameter_server_{std::move(parameter_server)},
      registry_{std::move(registry)} {
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

void PhysicsSystem::PhysicsStep(const double delta_t) {
  // TODO:: break up delta_t if its too large. Old implementation below
  for (auto id : registry_->GetView<Acceleration, Velocity>()) {
    auto [acceleration, velocity] = registry_->GetComponents<Acceleration, Velocity>(id);
    velocity.x += acceleration.x * delta_t;
    velocity.x = std::min(velocity.x, velocity.max_x);
    velocity.x = std::max(velocity.x, -velocity.max_x);

    velocity.y += acceleration.y * delta_t;
    velocity.y = std::min(velocity.y, velocity.max_y);
    velocity.y = std::max(velocity.y, -velocity.max_y);
  }

  for (auto id : registry_->GetView<Velocity, Position, CollisionBox, Collision>()) {
    auto [velocity, position, collision_box, collisions] =
        registry_->GetComponents<Velocity, Position, CollisionBox, Collision>(id);

    Collision old_collisions = collisions;
    collisions = {};

    position.x += velocity.x * delta_t;
    this->CheckPlayerCollision(id, Axis::X);

    position.y += velocity.y * delta_t;
    this->CheckPlayerCollision(id, Axis::Y);

    UpdateCollisionsChanged(collisions, old_collisions);
  }

  for (auto id : registry_->GetView<Velocity, Position, Projectile>()) {
    auto [velocity, position] = registry_->GetComponents<Velocity, Position>(id);
    position.x += velocity.x * delta_t;
    position.y += velocity.y * delta_t;
    if(IsCollision(collisions_grid_, position.x, position.y)){
      registry_->RemoveComponent(id);
    }
  }
}

void PhysicsSystem::ApplyGravity() {
  const auto gravity = parameter_server_->GetParameter<double>("physics/gravity");
  for (auto id : registry_->GetView<Acceleration>()) {
    auto [acceleration] = registry_->GetComponents<Acceleration>(id);
    acceleration.y = -gravity;
  }
}

void PhysicsSystem::ApplyFriction(const double delta_t) {
  for (auto id :
       registry_->GetView<Acceleration, Velocity, Position, Collision, StateComponent>()) {
    auto [acceleration, velocity, position, collisions, state] =
        registry_->GetComponents<Acceleration, Velocity, Position, Collision, StateComponent>(id);
    if (acceleration.x != 0) {
      continue;
    }
    if (!collisions.bottom) {
      // Air drag: resistance is proportional to velocity.
      const auto air_friction = parameter_server_->GetParameter<double>("physics/air.friction");
      velocity.x -= velocity.x * air_friction * delta_t;
      continue;
    }
    std::string ground_friction_key = "physics/ground.friction";
    if (state.state.GetState() == State::BackDodgeShot) {
      ground_friction_key = "physics/slide.friction";
    }
    const auto ground_friction = parameter_server_->GetParameter<double>(ground_friction_key);
    if (std::abs(velocity.x) < ground_friction * delta_t) {
      velocity.x = 0;
      continue;
    }
    // Coulomb friction: Resistance is relative to normal force, independent of velocity.
    velocity.x -= ground_friction * delta_t * (velocity.x > 0 ? 1 : -1);
  }
}

void PhysicsSystem::SetFacingDirection() {
  for (const auto id : registry_->GetView<Acceleration, FacingDirection>()) {
    const auto [acceleration, facing] = registry_->GetComponents<Acceleration, FacingDirection>(id);
    if (acceleration.x != 0) {
      facing.facing = acceleration.x < 0 ? Direction::LEFT : Direction::RIGHT;
    }
  }
}

void PhysicsSystem::SetDistanceFallen(const double delta_t) {
  for (const auto id : registry_->GetView<Velocity, DistanceFallen>()) {
    auto [velocity, distance_fallen] = registry_->GetComponents<Velocity, DistanceFallen>(id);
    if (velocity.y < 0) {
      distance_fallen.distance_fallen += -1 * velocity.y * delta_t;
    }
  }
}

// void PhysicsSystem::PhysicsStep(Player& player) {
//   const auto now = GameClock::NowGlobal();
//   const double delta_t = (now - player.last_update).count() / 1e9;
//   Collisions old_collisions = player.collisions;

//   if (delta_t > 0.1) {
//     // Collision detection will not work if the game is running very slow (<10hz).
//     // Therefore break it up into many smaller steps.
//     const int num_steps = static_cast<int>(delta_t / 0.02);
//     const double delta_t_fraction = delta_t / num_steps;
//     for (int i = 0; i < num_steps; ++i) {
//       PhysicsStep(delta_t_fraction, player);
//     }
//   } else {
//     PhysicsStep(delta_t, player);
//   }

//   UpdateCollisionsChanged(player.collisions, old_collisions);

//   player.last_update = now;
// }

void PhysicsSystem::CheckPlayerCollision(EntityId id, const Axis& axis) {
  auto [position, collision_box, collisions] =
      registry_->GetComponents<Position, CollisionBox, Collision>(id);
  const auto [lower_collision, upper_collision] = CheckAxisCollision(position, collision_box, axis);

  if (lower_collision && upper_collision) {
    const std::string axis_str = axis == Axis::X ? "Horizontal" : "Vertical";
    LOG_ERROR(axis_str << " Squish!");
  }

  if (!lower_collision && !upper_collision) {
    return;
  }
  ResolveCollisions(id, axis, tile_size_, lower_collision, upper_collision);
}

}  // namespace platformer