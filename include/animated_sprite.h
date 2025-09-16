#pragma once

#include <olcPixelGameEngine.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "utils/game_clock.h"

namespace platformer {

class AnimatedSprite {
 public:
  static std::optional<AnimatedSprite> CreateAnimatedSprite(
      const std::filesystem::path& sprite_sheet_path,
      int sprite_width,
      bool loops,
      bool forwards_backwards,
      int animation_duration_ms);

  void StartAnimation();

  // Returns true if it is a non looping sprite and there are no frames left.
  [[nodiscard]] bool Expired() const;

  [[nodiscard]] const olc::Sprite* GetFrame() const;

 private:
  AnimatedSprite() = default;

  bool loops_;
  bool forwards_backwards_;
  int frame_delay_ms_;
  std::vector<std::unique_ptr<olc::Sprite>> frames_;
  int frame_count_;
  TimePoint start_time_;
};

}  // namespace platformer