#pragma once

#include "olcPixelGameEngine.h"
namespace platformer {

struct Vector2d {
  double x{};
  double y{};
};

struct Vector2i {
  int x{};
  int y{};
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


}  // namespace platformer