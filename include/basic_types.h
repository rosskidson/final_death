#pragma once

#include "olcPixelGameEngine.h"
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


}  // namespace platformer