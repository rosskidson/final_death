#pragma once

#include <chrono>
namespace platformer {

// TODO:: Not yet used in the code.
class GameClock {
 public:
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;

  GameClock() : start_{clock::now()}, paused_{false}, pause_offset_{0} {}

  // ---- Instance methods ----
  time_point now() const {
    if (paused_) return paused_at_;
    return clock::now() - pause_offset_;
  }

  void pause() {
    if (!paused_) {
      paused_at_ = now();
      paused_ = true;
    }
  }

  void resume() {
    if (paused_) {
      pause_offset_ += clock::now() - paused_at_;
      paused_ = false;
    }
  }

  bool is_paused() const { return paused_; }

  // ---- Static global default instance ----
  static GameClock& global() {
    static GameClock instance;
    return instance;
  }

  static time_point now_global() { return global().now(); }
  static void pause_global() { global().pause(); }
  static void resume_global() { global().resume(); }
  static bool is_paused_global() { return global().is_paused(); }

 private:
  time_point start_;
  bool paused_;
  duration pause_offset_;
  time_point paused_at_;
};

}  // namespace platformer