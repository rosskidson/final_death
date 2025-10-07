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
};

struct Acceleration {
  double x{};
  double y{};
};

struct FacingDirection {
  Direction facing{Direction::RIGHT};
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

struct State {
  PlayerState state;
  TimePoint state_set_at;
  std::set<PlayerState> requested_states;
};

struct PlayerComponent {
  Vector2d cached_velocity{};
  double distance_fallen{};
  //   TimePoint roll_start_time{};
  AnimationManager animation_manager;
};

}  // namespace platformer
