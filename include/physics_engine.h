#pragma once

#include "basic_types.h"
#include "game_configuration.h"
#include "player.h"
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
  PhysicsEngine(const Level& level, std::shared_ptr<ParameterServer> parameter_server);

  void PhysicsStep(Player& player);

  [[nodiscard]] AxisCollisions CheckPlayerAxisCollision(const Player& player, Axis axis) const;

 private:
  void PhysicsStep(const double delta_t, Player& player);
  void CheckPlayerCollision(Player& player, const Axis& axis) const;

  int tile_size_;
  Grid<int> collisions_grid_;
  std::shared_ptr<ParameterServer> parameter_server_;
};

}  // namespace platformer