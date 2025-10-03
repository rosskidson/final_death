#pragma once

#include <olcPixelGameEngine.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "utils/chrono_helpers.h"

namespace platformer {

class AnimatedSprite {
 public:
  static std::optional<AnimatedSprite> CreateAnimatedSprite(
      const std::filesystem::path& sprite_sheet_path,
      bool loops,
      int start_frame_idx = 0,
      int end_frame_idx = -1,  // Includes this frame idx, i.e. 0 to 4 means 5 frames.
      int intro_frames = -1,
      bool forwards_backwards = false);

  void StartAnimation();
  void StartAnimation(const TimePoint& start_time);

  [[nodiscard]] TimePoint GetStartTime() const { return start_time_; }

  // Returns true if it is a non looping sprite and there are no frames left.
  [[nodiscard]] bool Expired() const;

  [[nodiscard]] const olc::Sprite* GetFrame() const;

  void TriggerCallbacks();

  // Add a callback to be triggered when a certain frame is reached.
  void AddCallback(int frame_idx, std::function<void()> callback);

  // Add a callback to be triggered when the animation expires (only for non-looping animations).
  void AddExpireCallback(std::function<void()> callback);

  [[nodiscard]] int GetTotalAnimationTimeMs() const;

 private:
  AnimatedSprite() = default;
  [[nodiscard]] int GetCurrentFrameIdx() const;

  bool loops_;
  bool forwards_backwards_;
  int intro_frames_;
  std::vector<std::unique_ptr<olc::Sprite>> frames_;
  std::vector<int> frame_timing_;
  std::vector<int> frame_timing_lookup_;
  TimePoint start_time_;

  std::vector<std::vector<std::function<void()>>> callbacks_;
  std::vector<bool> callback_triggered_;
  std::vector<std::function<void()>> expire_callbacks_;
  bool expire_callback_triggered_{false};
};

}  // namespace platformer