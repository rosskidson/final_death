#pragma once

#include <string>

namespace platformer {

struct Vector2d {
  double x{};
  double y{};
};

struct Vector2i {
  int x{};
  int y{};
};

inline constexpr auto operator-(const Vector2d& lhs, const Vector2d& rhs) {
  return Vector2d{lhs.x - rhs.x, lhs.y - rhs.y};
}

inline constexpr auto operator+(const Vector2d& lhs, const Vector2d& rhs) {
  return Vector2d{lhs.x + rhs.x, lhs.y + rhs.y};
}

struct BoundingBox {
  double left{};
  double right{};
  double bottom{};
  double top{};
};

enum class Direction : uint8_t { LEFT, RIGHT, UP, DOWN };

inline std::string ToString(Direction direction) {
  switch (direction) {
    case Direction::LEFT:
      return "LEFT";
    case Direction::RIGHT:
      return "RIGHT";
    case Direction::UP:
      return "UP";
    case Direction::DOWN:
      return "DOWN";
  }
  return "unknown direction";
}

enum class Weapon : uint8_t {Rifle, Shotgun, SIZE};

inline std::string ToString(Weapon weapon) {
  switch (weapon) {
    case Weapon::Rifle:
      return "Rifle";
    case Weapon::Shotgun:
      return "Rifle";
    case Weapon::SIZE:
      return "Not a weapon";
  }
  return "unknown weapon";
}


}  // namespace platformer