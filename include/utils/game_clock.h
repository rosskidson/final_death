#pragma once

#include <chrono>

namespace platformer {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

// Wraps std::chrono::steady_clock and enables pausing of the clock.
// This is both an instance and a singleton static global

class GameClock {
 public:
  GameClock() : start_{Clock::now()}, paused_{false}, pause_offset_{0} {}

  // ---- Instance methods ----
  [[nodiscard]] TimePoint Now() const {
    if (paused_) {
      return paused_at_ - pause_offset_;
    }
    return Clock::now() - pause_offset_;
  }

  void Pause() {
    if (!paused_) {
      paused_at_ = Clock::now();
      paused_ = true;
    }
  }

  void Resume() {
    if (paused_) {
      pause_offset_ += Clock::now() - paused_at_;
      paused_ = false;
    }
  }

  [[nodiscard]] bool IsPaused() const { return paused_; }

  // ---- Static global default instance ----
  static GameClock& Global() {
    static GameClock instance;
    return instance;
  }

  static TimePoint NowGlobal() { return Global().Now(); }
  static void PauseGlobal() { Global().Pause(); }
  static void ResumeGlobal() { Global().Resume(); }
  static bool IsPausedGlobal() { return Global().IsPaused(); }

 private:
  TimePoint start_;
  bool paused_;
  Duration pause_offset_;
  TimePoint paused_at_;
};

}  // namespace platformer