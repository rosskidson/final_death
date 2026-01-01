#pragma once

#include "chrono_helpers.h"

namespace platformer {

// Wraps std::chrono::steady_clock and enables pausing of the clock.
// This is both an instance and a singleton static global

class GameClock {
 public:
  GameClock(const double scale = 1.0)
      : start_{Clock::now()}, paused_{false}, pause_offset_{0}, scale_{scale} {}

  [[nodiscard]] TimePoint ScaledNow() const {
    Duration raw = Clock::now() - start_;
    const auto scaled = std::chrono::duration<double>(raw) * scale_;
    return start_ + std::chrono::duration_cast<Duration>(scaled);
  }

  // ---- Instance methods ----
  [[nodiscard]] TimePoint Now() const {
    if (paused_) {
      return paused_at_ - pause_offset_;
    }
    return ScaledNow() - pause_offset_;
  }

  void Pause() {
    if (!paused_) {
      paused_at_ = ScaledNow();
      paused_ = true;
    }
  }

  void Resume() {
    if (paused_) {
      pause_offset_ += ScaledNow() - paused_at_;
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
  double scale_;
};

}  // namespace platformer