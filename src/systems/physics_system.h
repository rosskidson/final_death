#pragma once

#include <memory>

#include "common_types/basic_types.h"
#include "common_types/components.h"
#include "common_types/game_configuration.h"
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

class PhysicsSystem {
 public:
  PhysicsSystem(const Level& level,
                std::shared_ptr<ParameterServer> parameter_server,
                std::shared_ptr<Registry> registry);

  void PhysicsStep(double delta_t);
  void ApplyGravity();
  void ApplyFriction(double delta_t);
  void SetDistanceFallen(double delta_t);

  [[nodiscard]] AxisCollisions CheckAxisCollision(const Position& position,
                                                  const CollisionBox& bounding_box,
                                                  Axis axis) const;

 private:
  void PhysicsStepImpl(double delta_t);
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