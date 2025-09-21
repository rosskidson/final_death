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
      bool loops,
      int start_frame_idx = 0,
      int end_frame_idx = -1,
      bool forwards_backwards=false);

  void StartAnimation();

  // Returns true if it is a non looping sprite and there are no frames left.
  [[nodiscard]] bool Expired() const;

  [[nodiscard]] const olc::Sprite* GetFrame() const;

  void TriggerCallbacks();

  void AddCallback(int frame_idx, std::function<void()> callback);

 private:
  AnimatedSprite() = default;
  [[nodiscard]] int GetCurrentFrameIdx() const;

  bool loops_;
  bool forwards_backwards_;
  std::vector<std::unique_ptr<olc::Sprite>> frames_;
  std::vector<int> frame_timing_;
  std::vector<int> frame_timing_lookup_;
  TimePoint start_time_;

  std::vector<std::vector<std::function<void()>>> callbacks_;
  std::vector<bool> callback_triggered_;
};

}  // namespace platformer