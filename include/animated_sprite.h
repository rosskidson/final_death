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
  // sprite_sheet_path: Path to the .png spritesheet. This also expects a metadata file with
  //                    the same name but with a .json extension.
  // loops:           True to loop the animation, False it will 'expire' after completion
  // start_frame_idx: If greater than zero, those frames will be skipped when loading
  // end_frame_idx:   As per start_frame_idx. Includes this frame idx, i.e. 0 to 4 means 5 frames
  // intro_frames:    For looping only: play the first n frames only once the first time through.
  //                  e.g. '2' = first 3 frames are played the first time, then loops starting at 3.
  //                  Applies to indices after start/end have been applied.
  // forwards_backwards: Will play the frames from start to end, then from end back to start.
  //
  static std::optional<AnimatedSprite> CreateAnimatedSprite(
      const std::filesystem::path& sprite_sheet_path,
      bool loops,
      int start_frame_idx = 0,
      int end_frame_idx = -1,
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
  // The index is after the start/end frame idx range has been applied.
  // E.g. start_frame_idx = 1, Callback on 2nd frame, frame_idx = 1.
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