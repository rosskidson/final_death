#pragma once

#include <vector>

#include "basic_types.h"
#include "utils/game_clock.h"

namespace platformer {

class AnimatedSprite {
 public:
  AnimatedSprite(const std::string& sprite_sheet_path, int sprite_width, bool loops);

  void StartAnimation();

  // Returns true if it is a non looping sprite and there are no frames left.
  bool Expired();

  [[nodiscard]] const olc::Sprite* GetFrame() const;

 private:
  bool loops_;
  std::vector<olc::Sprite> frames_;
  int frame_count_;
  TimePoint start_time_;
};

}  // namespace platformer