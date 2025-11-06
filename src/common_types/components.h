#pragma once

#include <limits>
#include <set>

#include "common_types/actor_state.h"
#include "common_types/basic_types.h"
#include "utils/game_clock.h"

namespace platformer {

enum class Direction : uint8_t { LEFT, RIGHT };

inline std::string ToString(Direction direction) {
  return direction == Direction::LEFT ? "LEFT" : "RIGHT";
}

struct Position {
  double x{};
  double y{};
};

struct Velocity {
  double x{};
  double y{};

  double max_x{std::numeric_limits<double>::max()};
  double max_y{std::numeric_limits<double>::max()};
};

struct Acceleration {
  double x{};
  double y{};
};

struct CollisionBox {
  int x_offset_px{};  // Measured from the left
  int y_offset_px{};  // Measured from the bottom
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
  Actor actor_type;  // Consider adding Components for each actor type instead.
  StateAccess state;
};

struct PlayerComponent {
  std::set<State> requested_states;
  Vector2d cached_velocity{};
};

struct DistanceFallen {
  double distance_fallen{};
};

struct Projectile {};

struct Particle {};

struct TimeToDespawn {
  TimeToDespawn() = default;
  TimeToDespawn(double seconds): time_to_despawn(GameClock::NowGlobal() + FromSecs(seconds)) {}
  TimePoint time_to_despawn{};
};

}  // namespace platformer
