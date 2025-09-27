#pragma once

#include <set>
#include "animation_manager.h"
#include "basic_types.h"
#include "utils/game_clock.h"
#include "player_state.h"

namespace platformer {

struct Collisions {
  bool left, right, top, bottom;
  bool left_changed, right_changed, top_changed, bottom_changed;
};

struct Player {
  Vector2d position{};
  Vector2d velocity{};
  Vector2d acceleration{};

  Vector2d cached_velocity{};

  TimePoint last_update{GameClock::NowGlobal()};

  int x_offset_px{};
  int y_offset_px{};
  int collision_width_px{};
  int collision_height_px{};

  PlayerState state;
  AnimationManager animation_manager;

  std::set<PlayerState> requested_states;

  Collisions collisions{};

  bool facing_left{};

  double distance_fallen{};

  TimePoint roll_start_time{};
};

// void SetPlayerState(Player& player, PlayerState new_state);

}  // namespace platformer