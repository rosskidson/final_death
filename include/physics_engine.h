#pragma once

#include "basic_types.h"
#include "game_configuration.h"
#include "player.h"
#include "utils/parameter_server.h"

namespace platformer {

enum class Side : std::uint8_t { LEFT, RIGHT, TOP, BOTTOM };
enum class Axis : std::uint8_t { X, Y };

class PhysicsEngine {
 public:
  PhysicsEngine(const Level& level, std::shared_ptr<ParameterServer> parameter_server);

  void PhysicsStep(Player& player);

 private:
  void CheckPlayerCollision(Player& player, const Axis& axis);
  int tile_size_;
  Grid<int> collisions_grid_;
  std::shared_ptr<ParameterServer> parameter_server_;
};

}  // namespace platformer