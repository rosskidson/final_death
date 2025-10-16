#pragma once

#include <set>

// #include "animation_manager.h"
#include "basic_types.h"
#include "state.h"
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

struct CommonState {
  Actor actor_type;
  std::shared_ptr<StateInterface> state;
  // TODO:: TimePoint should only be updated when state is set and new state is different
  // This makes it an invariant. Ask Mr. GPT about this one.
  TimePoint state_set_at; 
};

struct PlayerState {
  std::set<PlayerState> requested_states;
  std::shared_ptr<PlayerStateAccess> state;
  Vector2d cached_velocity{};
};

struct DistanceFallen {
  double distance_fallen{};
};

// struct Animation {
//   TimePoint start_time;
//   SpriteKey current_animation;
// };

}  // namespace platformer
