#pragma once

#include "animation_manager.h"
#include "olcPixelGameEngine.h"
#include "utils/game_clock.h"
namespace platformer {

// TODO:: Probably replace with Eigen
struct Vector2d {
  double x{};
  double y{};
};

struct BoundingBox {
  double left{};
  double right{};
  double bottom{};
  double top{};
};

inline constexpr auto operator-(const Vector2d& lhs, const Vector2d& rhs) {
  return Vector2d{lhs.x - rhs.x, lhs.y - rhs.y};
}

inline constexpr auto operator+(const Vector2d& lhs, const Vector2d& rhs) {
  return Vector2d{lhs.x + rhs.x, lhs.y + rhs.y};
}

struct Collisions {
  bool left, right, top, bottom;
};

struct Player {
  Vector2d position{};
  Vector2d velocity{};
  Vector2d acceleration{};

  TimePoint last_update{GameClock::NowGlobal()};

  int x_offset_px{};
  int y_offset_px{};
  int collision_width_px{};
  int collision_height_px{};

  AnimationManager animation_manager;

  // TODO:: THIS IT NOT HOW I WANT IT TO END UP.
  // olc::Sprite* sprite;

  // This is just for debug/visualizations.
  Collisions collisions{};

  bool facing_left{};
  TimePoint started_shooting;

  // int animation_frame = 0;
  // TimePoint animation_update{GameClock::NowGlobal()};
};

}  // namespace platformer