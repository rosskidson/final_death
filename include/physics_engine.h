#pragma once

#include <memory>

#include "basic_types.h"
#include "components.h"
#include "game_configuration.h"
#include "player.h"
#include "registry.h"
#include "registry_helpers.h"
#include "utils/parameter_server.h"

namespace platformer {

enum class Side : std::uint8_t { LEFT, RIGHT, TOP, BOTTOM };
enum class Axis : std::uint8_t { X, Y };

struct AxisCollisions {
  bool lower_collision{false};
  bool upper_collision{false};
};

class PhysicsEngine {
 public:
  PhysicsEngine(const Level& level,
                std::shared_ptr<ParameterServer> parameter_server,
                std::shared_ptr<Registry> registry);

  // void PhysicsStep(Player& player);
  void PhysicsStep(double delta_t);

  [[nodiscard]] AxisCollisions CheckAxisCollision(const Position& position,
                                                  const CollisionBox& bounding_box,
                                                  Axis axis) const;
  void GravitySystem();
  void FrictionSystem(const double& delta_t);
  void PhysicsSystem(const double& delta_t);

 private:
  // void PhysicsStep(const double delta_t, Player& player);
  void CheckPlayerCollision(EntityId id, const Axis& axis);
  void ResolveCollisions(EntityId id,
                         const Axis& axis,
                         int tile_size,
                         bool lower_collision,
                         bool upper_collision);

  int tile_size_;
  Grid<int> collisions_grid_;
  std::shared_ptr<ParameterServer> parameter_server_;
  std::shared_ptr<Registry> registry_;
};

}  // namespace platformer