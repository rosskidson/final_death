#pragma once

#include <set>

// #include "animation_manager.h"
#include "basic_types.h"
#include "actor_state.h"

namespace platformer {

enum class Direction : uint8_t { LEFT, RIGHT };

struct Position {
  double x{};
  double y{};
};

struct Velocity {
  double x{};
  double y{};

  double max_x{};
  double max_y{};
};

struct Acceleration {
  double x{};
  double y{};
};

struct CollisionBox {
  int x_offset_px{};
  int y_offset_px{};
  int collision_width_px{};
  int collision_height_px{};
};

struct Collision {
  bool left, right, top, bottom;
  bool left_changed, right_changed, top_changed, bottom_changed;
};

struct FacingDirection {
  Direction facing{Direction::RIGHT};
};

struct StateComponent {
  Actor actor_type;
  StateAccess state;
};

struct PlayerComponent {
  std::set<State> requested_states;
  Vector2d cached_velocity{};
};

struct DistanceFallen {
  double distance_fallen{};
};

}  // namespace platformer
