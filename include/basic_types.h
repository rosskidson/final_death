#pragma once

// TODO:: Probably replace with Eigen

#include "olcPixelGameEngine.h"
namespace platformer {

struct Vector2d {
  double x{};
  double y{};
};

inline constexpr auto operator-(const Vector2d& lhs, const Vector2d& rhs) {
  return Vector2d{lhs.x - rhs.x, lhs.y - rhs.y};
}

inline constexpr auto operator+(const Vector2d& lhs, const Vector2d& rhs) {
  return Vector2d{lhs.x + rhs.x, lhs.y + rhs.y};
}

struct Bounds {
  double max_x{};
  double min_x{};
  double max_y{};
  double min_y{};
};

struct Player {
  Vector2d position{};
  Vector2d velocity{};

  // TODO:: THIS IT NOT HOW I WANT IT TO END UP.
  int sprite_width_px{};
  int sprite_height_px{};
  olc::Sprite sprite;
};

}  // namespace platformer