#pragma once

#include <set>

#include "animation_manager.h"
#include "basic_types.h"
#include "player_state.h"
#include "utils/chrono_helpers.h"

namespace platformer {

enum class Direction : uint8_t { LEFT, RIGHT };

struct Position {
  double x{};
  double y{};
};

struct Velocity {
  double x{};
  double y{};

  double max_x{100};  // TODO:: fix this
  double max_y{100};  // TODO:: fix this
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

struct State {
  PlayerState state;
  TimePoint state_set_at;
  std::set<PlayerState> requested_states;
};

struct DistanceFallen {
  double distance_fallen{};
};

// TODO:: Remove this component
struct PlayerComponent {
  Vector2d cached_velocity{};
  // TODO:: Think about animation manager in the context of ECS pattern.
  // Should it be a separate system?
  // Or just its own component?
  AnimationManager animation_manager;
};

}  // namespace platformer
