#pragma once

#include <olcPixelGameEngine.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "common_types/sprite.h"
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

  // If the animation has ended and it is not looping, the last frame will be returned.
  [[nodiscard]] Sprite GetFrame(TimePoint start_time) const;

  // Add an event to be triggered and emitted when a certain frame is reached.
  // The event may be specified as a free form string.
  // The index is after the start/end frame idx range has been applied.
  // E.g. start_frame_idx = 1, Callback on 2nd frame, frame_idx = 1.
  //
  // An event "AnimationEnded" will always be emitted when a non looping animation has finished.
  void AddEventSignal(int frame_idx, const std::string& event_name);

  // As above, but trigger after a non looping animation has finished.
  void AddExpiredEventSignal(const std::string& event_name);

  [[nodiscard]] std::vector<std::string> GetAnimationEvents(TimePoint start_time,
                                                            int& last_animation_frame) const;

  [[nodiscard]] int GetTotalAnimationTimeMs() const;

 private:
  AnimatedSprite() = default;

  // Returns -1 if it is non loop and the animation has ended.
  [[nodiscard]] int GetCurrentFrameIdx(TimePoint start_time) const;

  bool loops_;
  bool forwards_backwards_;
  int intro_frames_;
  int draw_offset_x;
  int draw_offset_y;
  std::vector<std::unique_ptr<olc::Sprite>> frames_;
  std::vector<int> frame_timing_;
  std::vector<int> frame_timing_lookup_;

  std::vector<std::vector<std::string>> signals_to_emit_;
  std::vector<std::string> signals_to_emit_on_expiration_;
};

}  // namespace platformer