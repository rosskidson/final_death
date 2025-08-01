#pragma once

// TODO:: Probably replace with Eigen

namespace platformer {

struct Vector2d {
  double x{};
  double y{};
};

struct Bounds {
  double max_x{};
  double min_x{};
  double max_y{};
  double min_y{};
};

}  // namespace platformer