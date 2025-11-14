#pragma once

#include <limits>
#include <set>

#include "animation/animation_frame_index.h"
#include "common_types/actor_state.h"
#include "common_types/basic_types.h"
#include "utils/game_clock.h"

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
  State state{};
};

struct PlayerComponent {
  std::set<State> requested_states;
  Vector2d cached_velocity{};
  Weapon weapon{Weapon::Rifle};
};

// **** Sprite refactor ******
//
// All the animation state will be moved/copied from StateComponent to AnimatedSpriteComponent
//
// 1. Remove last_animation_frame_idx from StateAccess               (DONE)
// 2. Add a function after UpdatePlayerState that updates the AnimatedSpriteComponent
//     This will contain special setting logic
//        - transition from flyingshoot -> standingshoot -> don't reset start time
//        - Reset timer if shooting and animation expired
// 3. Rendering only iterates over AnimatedSprite components         (DONE)
// 4. AnimationManager also now knows nothing about StateComponent   (DONE)
// 
//  You removed the state object and state set at, but you need this after all to 
//  transition between roll states.  Add it back
//
//   TEST
//
// 5. Rename animation manager to sprite manager
// 6. Add a sprite map
// 7. Get sprite simply checks if the entity has animated or non animated sprite, then
//     looks in the corresponding map.

struct SpriteComponent {
  std::string key;
};

struct AnimatedSpriteComponent {
  TimePoint start_time{GameClock::NowGlobal()};
  AnimationFrameIndex last_animation_frame_idx{};
  std::string key{};
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
