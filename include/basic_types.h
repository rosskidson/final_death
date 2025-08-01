#pragma once

#include <chrono>

#include "olcPixelGameEngine.h"
namespace platformer {

// TODO:: Probably replace with Eigen
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

struct Player {
  Vector2d position{};
  Vector2d velocity{};
  Vector2d acceleration{};

  std::chrono::time_point<std::chrono::system_clock> last_update{};

  // TODO:: THIS IT NOT HOW I WANT IT TO END UP.
  olc::Sprite* sprite;
};

}  // namespace platformer