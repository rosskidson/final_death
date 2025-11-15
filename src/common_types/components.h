#pragma once

#include <functional>
#include <limits>
#include <set>

#include "animation/animation_frame_index.h"
#include "common_types/actor_state.h"
#include "common_types/basic_types.h"
#include "utils/game_clock.h"

namespace olc {
class PixelGameEngine;
}

namespace platformer {

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
  Actor actor_type{};
  StateAccess state{State::Idle};
};

struct PlayerComponent {
  std::set<State> requested_states;
  Vector2d cached_velocity{};
  Weapon weapon{Weapon::Rifle};
};

struct SpriteComponent {
  std::string key;
};

struct AnimatedSpriteComponent {
  TimePoint start_time{GameClock::NowGlobal()};
  AnimationFrameIndex last_animation_frame_idx{};
  std::string key{};
};

struct DrawFunction {
  // Arguments are pixel coordinates of the entity's position and an olc engine ptr
  // (Entity must also have a Position Component)
  std::function<void(int, int, olc::PixelGameEngine*)> draw_fn;
};

struct DistanceFallen {
  double distance_fallen{};
};

struct Projectile {};

struct Particle {};

struct TimeToDespawn {
  TimeToDespawn() = default;
  TimeToDespawn(double seconds) : time_to_despawn(GameClock::NowGlobal() + FromSecs(seconds)) {}
  TimePoint time_to_despawn{};
};

}  // namespace platformer
