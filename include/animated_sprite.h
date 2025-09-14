#pragma once

#include <olcPixelGameEngine.h>

#include <memory>
#include <string>
#include <vector>

#include "utils/game_clock.h"

namespace platformer {

class AnimatedSprite {
 public:
  // TODO:: remove this.
  AnimatedSprite() = default;

  AnimatedSprite(const std::string& sprite_sheet_path,
                 int sprite_width,
                 bool loops,
                 bool forwards_backwards,
                 int frame_delay_ms);

  void StartAnimation();

  // Returns true if it is a non looping sprite and there are no frames left.
  bool Expired() const;

  // I wish this was const but olc needs non const sprites (vom).
  [[nodiscard]] olc::Sprite* GetFrame();

 private:
  bool loops_;
  bool forwards_backwards_;
  int frame_delay_ms_;
  std::vector<std::unique_ptr<olc::Sprite>> frames_;
  int frame_count_;
  TimePoint start_time_;
};

}  // namespace platformer