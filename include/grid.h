#pragma once

#include <utils/check.h>


#include <vector>

namespace platformer {

template <typename T>
class Grid {
 public:
  Grid() = default;
  Grid(const int width, const int height) : width_(width), height_(height) {
    grid_.resize(width * height);
  }

  void SetTile(const int x, const int y, T value) {
    CHECK(x < width_ && y < height_ && x >= 0 && y >= 0);
    grid_[y * width_ + x] = value;
  }

  void SetTile(const int flattened_coord, T value) {
    CHECK(flattened_coord >= 0 && flattened_coord < grid_.size());
    grid_[flattened_coord] = value;
  }

  T GetTile(const int x, const int y) const {
    CHECK(x < width_ && y < height_ && x >= 0 && y >= 0);
    return grid_[y * width_ + x];
  }

  int GetWidth() const { return width_; };
  int GetHeight() const { return height_; };

 private:
  int width_;
  int height_;
  std::vector<T> grid_;
};

}  // namespace platformer