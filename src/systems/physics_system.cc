#include "physics_system.h"

#include <algorithm>
#include <unordered_map>

#include "common_types/actor_state.h"
#include "common_types/basic_types.h"
#include "common_types/components.h"
#include "common_types/entity.h"
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

void ResolvePointCollision(const Grid<int>& collision_grid,
                           const Axis axis,
                           Position& position,
                           Velocity& velocity) {
  const int int_x = static_cast<int>(std::floor(position.x));
  const int int_y = static_cast<int>(std::floor(position.y));
  if (!IsCollision(collision_grid, position.x, position.y)) {
    return;
  }
  if (axis == Axis::X) {
    position.x = std::round(position.x);
    velocity.x = -velocity.x;
    return;
  }
  position.y = std::round(position.y);
  velocity.x = 0;
  velocity.y = 0;
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
      occupancy_grid_{collisions_grid_.GetWidth(), collisions_grid_.GetHeight()},
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

std::vector<Axis> PhysicsSystem::MoveParticleCheckCollision(const EntityId id,
                                                            const double delta_t) {
  auto [velocity, position] = registry_->GetComponents<Velocity, Position>(id);
  Position new_position{position.x + velocity.x * delta_t,  //
                        position.y + velocity.y * delta_t};
  const auto old_x = static_cast<int>(std::floor(position.x));
  const auto old_y = static_cast<int>(std::floor(position.y));
  auto new_x = static_cast<int>(std::floor(new_position.x));
  auto new_y = static_cast<int>(std::floor(new_position.y));

  if (old_x == new_x && old_y == new_y) {
    std::swap(position, new_position);
    return {};
  }
  if (!IsCollision(collisions_grid_, new_position.x, new_position.y)) {
    std::swap(position, new_position);
    return {};
  }
  // Handle traversal of multiple cells later
  RB_CHECK(std::abs(new_x - old_x) < 2 && std::abs(new_y - old_y) < 2);
  if (std::abs(new_x - old_x) != 0) {
    if (velocity.x < 0) {
      new_x += 1;
    }
    const double x_dist = new_x - position.x;
    const double gradient = (new_position.y - position.y) / (new_position.x - position.x);
    new_position.x = new_x - std::copysign(1e-3, velocity.x);
    new_position.y = position.y + gradient * x_dist;
    std::swap(position, new_position);
    return {Axis::X};
  }
  // TODO we don't handle x and y together
  if (velocity.y < 0) {
    new_y += 1;
  }
  const double y_dist = new_y - position.y;
  const double gradient = (new_position.x - position.x) / (new_position.y - position.y);
  new_position.x = position.x + gradient * y_dist;
  new_position.y = new_y - std::copysign(1e-3, velocity.y);
  std::swap(position, new_position);
  return {Axis::Y};
}

void PhysicsSystem::PhysicsStepImpl(const double delta_t) {
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
    this->CheckCollisionBox(id, Axis::X);

    position.y += velocity.y * delta_t;
    this->CheckCollisionBox(id, Axis::Y);

    UpdateCollisionsChanged(collisions, old_collisions);
  }

  for (auto id : registry_->GetView<Velocity, Position, Projectile>()) {
    const auto axes = MoveParticleCheckCollision(id, delta_t);
    auto [velocity, position] = registry_->GetComponents<Velocity, Position>(id);
    // position.x += velocity.x * delta_t;
    // position.y += velocity.y * delta_t;

    if (axes.empty()) {
      continue;
    }

    // Doesn't handle both
    if (axes.front() == Axis::X) {
      velocity.x *= -1;
    } else {
      velocity.y *= -1;
    }

    if (registry_->HasComponent<FacingDirection>(id)) {
      auto& facing = registry_->GetComponent<FacingDirection>(id).facing;
      if (facing == Direction::UP) {
        facing = Direction::DOWN;
      } else if (facing == Direction::DOWN) {
        facing = Direction::UP;
      } else if (facing == Direction::LEFT) {
        facing = Direction::RIGHT;
      } else {
        facing = Direction::LEFT;
      }
    }

    // if (IsCollision(collisions_grid_, position.x, position.y)) {
    // Spawn particles
    for (int i = 0; i < 5; ++i) {
      Position particle_pos = position;
      Velocity particle_vel = velocity;
      // ResolvePointCollision(collisions_grid_, Axis::X, particle_pos, particle_vel);
      // ResolvePointCollision(collisions_grid_, Axis::Y, particle_pos, particle_vel);
      // int x_sign = velocity.x > 0 ? 1 : -1;
      // int y_sign = velocity.y > 0 ? 1 : -1;
      // TODO(BT-18):: use rng
      particle_vel.x = std::copysign((rand() % 50) / 10., velocity.x);
      particle_vel.y = std::copysign((rand() % 50) / 10., velocity.y);
      DrawFunction draw_function{};
      const uint8_t color = 128 + (rand() % 128);
      draw_function.draw_fn = [color](int px, int py, olc::PixelGameEngine* engine_ptr) {
        engine_ptr->Draw(px, py, olc::Pixel{color, color, color});
      };
      registry_->AddComponents(Acceleration{}, particle_vel, particle_pos, Particle{},
                               TimeToDespawn{0.5}, draw_function);
    }

    // registry_->RemoveComponent(id);
    // }
  }

  for (auto id : registry_->GetView<Velocity, Position, Particle>()) {
    bool collision{};
    auto [velocity, position] = registry_->GetComponents<Velocity, Position>(id);
    position.x += velocity.x * delta_t;
    ResolvePointCollision(collisions_grid_, Axis::X, position, velocity);
    position.y += velocity.y * delta_t;
    ResolvePointCollision(collisions_grid_, Axis::Y, position, velocity);
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
    if (*state.state == State::BackDodgeShot) {
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

void PhysicsSystem::SetDistanceFallen(const double delta_t) {
  for (const auto id : registry_->GetView<Velocity, DistanceFallen>()) {
    auto [velocity, distance_fallen] = registry_->GetComponents<Velocity, DistanceFallen>(id);
    if (velocity.y < 0) {
      distance_fallen.distance_fallen += -1 * velocity.y * delta_t;
    }
  }
}

void PhysicsSystem::PhysicsStep(const double delta_t) {
  std::unordered_map<EntityId, Collision> old_collisions;
  for (EntityId id : registry_->GetView<Collision>()) {
    old_collisions[id] = registry_->GetComponent<Collision>(id);
  }

  if (delta_t > 0.05) {
    // Collision detection will not work if the game is running very slow (<10hz).
    // Therefore break it up into many smaller steps.
    // NOTE: If physics is the reason it is running slow, this makes things worse.
    const int num_steps = static_cast<int>(delta_t / 0.02);
    const double delta_t_fraction = delta_t / num_steps;
    for (int i = 0; i < num_steps; ++i) {
      PhysicsStepImpl(delta_t_fraction);
    }
  } else {
    PhysicsStepImpl(delta_t);
  }

  for (EntityId id : registry_->GetView<Collision>()) {
    UpdateCollisionsChanged(registry_->GetComponent<Collision>(id), old_collisions[id]);
  }

  UpdateOccupancyGrid();
}

void PhysicsSystem::CheckCollisionBox(EntityId id, const Axis& axis) {
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

void PhysicsSystem::UpdateOccupancyGrid() {
  // TODO:: add reset to grid
  occupancy_grid_ = Grid<EntityId>(collisions_grid_.GetWidth(), collisions_grid_.GetHeight());
  for (const EntityId id : registry_->GetView<Position, CollisionBox>()) {
    auto bounding_box = GetBoundingBox(id);
    RB_CHECK(bounding_box.has_value());

    int min_x = bounding_box->left;
    int max_x = bounding_box->right;
    int min_y = bounding_box->bottom;
    int max_y = bounding_box->top;

    min_x = std::max(0, min_x);
    max_x = std::min(max_x, collisions_grid_.GetWidth() - 1);
    min_y = std::max(0, min_y);
    max_y = std::min(max_y, collisions_grid_.GetHeight() - 1);

    for (int i = min_x; i <= max_x; ++i) {
      for (int j = min_y; j <= max_y; ++j) {
        occupancy_grid_.SetTile(i, j, id);
      }
    }
  }
}

std::optional<BoundingBox> PhysicsSystem::GetBoundingBox(const EntityId id) const {
  if (!registry_->HasComponents<Position, CollisionBox>(id)) {
    return std::nullopt;
  }
  const auto& [position, collision_box] = registry_->GetComponentsConst<Position, CollisionBox>(id);
  const double bb_height = collision_box.collision_height_px / static_cast<double>(tile_size_);
  const double bb_width = collision_box.collision_width_px / static_cast<double>(tile_size_);
  const double bb_x_offset = collision_box.x_offset_px / static_cast<double>(tile_size_);
  const double bb_y_offset = collision_box.y_offset_px / static_cast<double>(tile_size_);

  return BoundingBox{
      position.x + bb_x_offset,
      position.x + bb_x_offset + bb_width,
      position.y + bb_y_offset,
      position.y + bb_y_offset + bb_height,
  };
}

bool PhysicsSystem::PointCollidesWithEntity(const Position& point, const EntityId id) {
  const auto bounding_box = GetBoundingBox(id);
  if (!bounding_box.has_value()) {
    return false;
  }

  return point.x >= bounding_box->left && point.x <= bounding_box->right &&
         point.y >= bounding_box->bottom && point.y <= bounding_box->top;
}

std::vector<CollisionEvent> PhysicsSystem::DetectProjectileCollisions() {
  std::vector<CollisionEvent> events;
  for (const EntityId id : registry_->GetView<Position, Projectile>()) {
    const auto& position = registry_->GetComponentConst<Position>(id);
    const int pos_x = static_cast<int>(std::floor(position.x));
    const int pos_y = static_cast<int>(std::floor(position.y));
    if (!occupancy_grid_.ValidCoord(pos_x, pos_y)) {
      continue;
    }
    const EntityId other_id = occupancy_grid_.GetTile(pos_x, pos_y);
    if (other_id == 0) {
      continue;
    }

    // LOG_INFO(id << ":\t potential Collision with " << other_id);
    if (PointCollidesWithEntity(position, other_id)) {
      events.emplace_back(CollisionEvent{other_id, id});
      // LOG_INFO(id << ":\t Collision with " << other_id);
    }
  }
  return events;
}

}  // namespace platformer